/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013-2018 Ivan Alonso (Kaian)
 ** Copyright (C) 2013-2018 Irontec SL. All rights reserved.
 **
 ** This program is free software: you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation, either version 3 of the License, or
 ** (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **
 ** You should have received a copy of the GNU General Public License
 ** along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **
 ****************************************************************************/
/**
 * @file sip_call.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage SIP call data
 *
 * This file contains the functions and structure to manage SIP call data
 *
 */

#include "sip_call.h"
#include "sip.h"
#include "setting.h"

sip_call_t *
call_create(char *callid, char *xcallid)
{
    sip_call_t *call;

    // Initialize a new call structure
    if (!(call = sng_malloc(sizeof(sip_call_t))))
        return NULL;

    // Create a vector to store call messages
    call->msgs = vector_create(2, 2);
    vector_set_destroyer(call->msgs, msg_destroyer);

    // Create an empty vector to store rtp packets
    if (setting_enabled(SETTING_CAPTURE_RTP)) {
        call->rtp_packets = vector_create(0, 40);
        vector_set_destroyer(call->rtp_packets, packet_destroyer);
    }

    // Create an empty vector to strore stream data
    call->streams = vector_create(0, 2);
    vector_set_destroyer(call->streams, vector_generic_destroyer);

    // Create an empty vector to store x-calls
    call->xcalls = vector_create(0, 1);

    // Initialize call filter status
    call->filtered = -1;

    // Set message callid
    call->callid = strdup(callid);
    call->xcallid = strdup(xcallid);

    return call;
}

void
call_destroy(sip_call_t *call)
{
    // Remove all call messages
    vector_destroy(call->msgs);
    // Remove all call streams
    vector_destroy(call->streams);
    // Remove all call rtp packets
    vector_destroy(call->rtp_packets);
    // Remove all xcalls
    vector_destroy(call->xcalls);
    // Deallocate call memory
    sng_free(call->callid);
    sng_free(call->xcallid);
    sng_free(call->reasontxt);
    sng_free(call->disconnect_by);
    sng_free(call->disconnect_code);
    sng_free(call);
}

void
call_destroyer(void *call)
{
    call_destroy((sip_call_t*)call);
}

bool
call_has_changed(sip_call_t *call)
{
    return call->changed;
}

void
call_add_message(sip_call_t *call, sip_msg_t *msg)
{
    // Set the message owner
    msg->call = call;
    // Put this msg at the end of the msg list
    msg->index = vector_append(call->msgs, msg);
    // Flag this call as changed
    call->changed = true;
}

void
call_add_stream(sip_call_t *call, rtp_stream_t *stream)
{
    // Store stream
    vector_append(call->streams, stream);
    // Flag this call as changed
    call->changed = true;
}

void
call_add_rtp_packet(sip_call_t *call, packet_t *packet)
{
    // Store packet
    vector_append(call->rtp_packets, packet);
    // Flag this call as changed
    call->changed = true;
}

int
call_msg_count(sip_call_t *call)
{
    return vector_count(call->msgs);
}

int
call_is_active(sip_call_t *call)
{
    return (call->state == SIP_CALLSTATE_CALLSETUP || call->state == SIP_CALLSTATE_INCALL);
}

int
call_is_invite(sip_call_t *call)
{
    sip_msg_t *first;
    if ((first = vector_first(call->msgs)))
        return (first->reqresp == SIP_METHOD_INVITE);

    return 0;
}

void
call_msg_retrans_check(sip_msg_t *msg)
{
    sip_msg_t *prev = NULL;
    vector_iter_t it;

    // Get previous message in call with same origin and destination
    it = vector_iterator(msg->call->msgs);
    vector_iterator_set_current(&it, vector_index(msg->call->msgs, msg));
    while ((prev = vector_iterator_prev(&it))) {
        if (addressport_equals(prev->packet->src, msg->packet->src) &&
                addressport_equals(prev->packet->dst, msg->packet->dst))
            break;
    }

    // Store the flag that determines if message is retrans
    if (prev && !strcasecmp(msg_get_payload(msg), msg_get_payload(prev))) {
        msg->retrans = prev;
    }
}

sip_msg_t *
call_msg_with_media(sip_call_t *call, address_t dst)
{
    sip_msg_t *msg;
    sdp_media_t *media;
    vector_iter_t itmsg;
    vector_iter_t itmedia;

    // Get message with media address configured in given dst
    itmsg = vector_iterator(call->msgs);
    while ((msg = vector_iterator_next(&itmsg))) {
        itmedia = vector_iterator(msg->medias);
        while ((media = vector_iterator_next(&itmedia))) {
            if (addressport_equals(dst, media->address)) {
                return msg;
            }
        }
    }

    return NULL;
}

void
call_update_state(sip_call_t *call, sip_msg_t *msg)
{
    int reqresp;

    if (!call_is_invite(call))
        return;

    // Get current message Method / Response Code
    reqresp = msg->reqresp;
    
    // CRITICAL FIX: Check for BYE regardless of state
    // BYE can come at any time and should always be processed
    if (reqresp == SIP_METHOD_BYE) {
        // BYE received - call is being terminated
        // Always mark as COMPLETED when BYE is received
        call->state = SIP_CALLSTATE_COMPLETED;
        call->cend_msg = msg;
        
        // Store disconnect info
        if (!call->disconnect_by) {
            char src_addr[256];
            sprintf(src_addr, "%s:%u", msg->packet->src.ip, msg->packet->src.port);
            call->disconnect_by = strdup(src_addr);
        }
        if (!call->disconnect_code) {
            call->disconnect_code = strdup("BYE");
        }
        // Continue processing to update other fields if needed
    }

    // If this message is actually a call, get its current state
    // Skip state changes if we just processed a BYE
    if (call->state && reqresp != SIP_METHOD_BYE) {
        if (call->state == SIP_CALLSTATE_CALLSETUP) {
            if (reqresp == SIP_METHOD_ACK) {
                // Check if this ACK matches the current INVITE transaction
                if (msg->cseq == call->invitecseq) {
                    // Find the most recent response to this INVITE
                    sip_msg_t *last_response = NULL;
                    int i;
                    for (i = call_msg_count(call) - 1; i >= 0; i--) {
                        sip_msg_t *m = vector_item(call->msgs, i);
                        if (!m) continue;
                        // Look for response with same cseq as this ACK
                        // Response CSeq should match the INVITE CSeq
                        if (m->cseq == msg->cseq && m->reqresp >= 100 && m->reqresp < 700) {
                            last_response = m;
                            break;
                        }
                    }
                    
                    if (last_response) {
                        if (last_response->reqresp >= 200 && last_response->reqresp < 300) {
                            // 2xx response - call is established
                            call->state = SIP_CALLSTATE_INCALL;
                            call->cstart_msg = msg;
                        } else if (last_response->reqresp == 407 || last_response->reqresp == 401) {
                            // Auth challenge - stay in CALLSETUP, waiting for new INVITE
                            // Do nothing, state remains CALLSETUP
                        } else {
                            // Other error response - call failed
                            // State should have been changed by the error response handler
                        }
                    } else {
                        // No response found for this ACK - might be timing issue
                        // Check if there's a 200 OK with ANY CSeq for INVITE
                        int i;
                        for (i = call_msg_count(call) - 1; i >= 0; i--) {
                            sip_msg_t *m = vector_item(call->msgs, i);
                            if (m && m->reqresp == 200) {
                                // Found a 200 OK, assume call is established
                                call->state = SIP_CALLSTATE_INCALL;
                                call->cstart_msg = msg;
                                break;
                            }
                        }
                    }
                }
                // ACK with different CSeq - ignore (probably for older transaction)
            } else if (reqresp == SIP_METHOD_CANCEL) {
                // Alice is not in the mood
                call->state = SIP_CALLSTATE_CANCELLED;
                // Store who sent the CANCEL - just IP:port
                if (!call->disconnect_by) {
                    char src_addr[256];
                    sprintf(src_addr, "%s:%u", msg->packet->src.ip, msg->packet->src.port);
                    call->disconnect_by = strdup(src_addr);
                }
                // Initial disconnect code (may be updated by 487 later)
                if (!call->disconnect_code) {
                    call->disconnect_code = strdup("CANCELLED");
                }
            } else if ((reqresp == 480) || (reqresp == 486) || (reqresp == 600)) {
                // Bob is busy - can happen even after 183/180 
                call->state = SIP_CALLSTATE_BUSY;
                // Store the busy response
                if (!call->disconnect_code) {
                    const char *resp_str = sip_get_msg_reqresp_str(msg);
                    if (resp_str) {
                        call->disconnect_code = strdup(resp_str);
                    } else {
                        char code_str[32];
                        sprintf(code_str, "%d", reqresp);
                        call->disconnect_code = strdup(code_str);
                    }
                }
                // Store who sent the busy response
                if (!call->disconnect_by) {
                    char src_addr[256];
                    sprintf(src_addr, "%s:%u", msg->packet->src.ip, msg->packet->src.port);
                    call->disconnect_by = strdup(src_addr);
                }
            } else if (reqresp == 603) {
                // Bob declined the call (603 Decline)
                call->state = SIP_CALLSTATE_REJECTED;
                // Store the decline response
                if (!call->disconnect_code) {
                    const char *resp_str = sip_get_msg_reqresp_str(msg);
                    if (resp_str) {
                        call->disconnect_code = strdup(resp_str);
                    } else {
                        call->disconnect_code = strdup("603 Decline");
                    }
                }
                // Store who declined (source of 603)
                if (!call->disconnect_by) {
                    char src_addr[256];
                    sprintf(src_addr, "%s:%u", msg->packet->src.ip, msg->packet->src.port);
                    call->disconnect_by = strdup(src_addr);
                }
            } else if (reqresp == 200) {
                // 200 OK - check if it's for INVITE
                // First check if CSeq matches current INVITE
                if (msg->cseq == call->invitecseq) {
                    // 200 OK for INVITE - call is established
                    call->state = SIP_CALLSTATE_INCALL;
                } else {
                    // CSeq mismatch - could be auth scenario
                    // Check if this is a 200 OK for any INVITE in this call
                    int i;
                    for (i = 0; i < call_msg_count(call); i++) {
                        sip_msg_t *m = vector_item(call->msgs, i);
                        if (m && m->reqresp == SIP_METHOD_INVITE && m->cseq == msg->cseq) {
                            // Found matching INVITE for this 200 OK
                            call->state = SIP_CALLSTATE_INCALL;
                            call->invitecseq = msg->cseq;  // Update to correct CSeq
                            break;
                        }
                    }
                }
            } else if (reqresp == 487 && call->invitecseq == msg->cseq) {
                // 487 Request Terminated - call was cancelled
                call->state = SIP_CALLSTATE_CANCELLED;
                // Store who terminated
                if (!call->disconnect_by) {
                    char src_addr[256];
                    sprintf(src_addr, "%s:%u", msg->packet->src.ip, msg->packet->src.port);
                    call->disconnect_by = strdup(src_addr);
                }
                // Store the 487 response
                if (!call->disconnect_code) {
                    const char *resp_str = sip_get_msg_reqresp_str(msg);
                    if (resp_str) {
                        call->disconnect_code = strdup(resp_str);
                    } else {
                        call->disconnect_code = strdup("487 Request Terminated");
                    }
                }
            } else if (reqresp > 400 && reqresp != 407 && reqresp != 401 && call->invitecseq == msg->cseq) {
                // Bob is not in the mood (ignore auth challenges 401/407 and 487 which is handled above)
                call->state = SIP_CALLSTATE_REJECTED;
                // Store the rejection response
                if (!call->disconnect_code) {
                    const char *resp_str = sip_get_msg_reqresp_str(msg);
                    if (resp_str) {
                        call->disconnect_code = strdup(resp_str);
                    } else {
                        char code_str[32];
                        sprintf(code_str, "%d", reqresp);
                        call->disconnect_code = strdup(code_str);
                    }
                    // In case of rejection, store destination IP
                    if (!call->disconnect_by) {
                        char dst_addr[256];
                        sprintf(dst_addr, "%s:%u", msg->packet->dst.ip, msg->packet->dst.port);
                        call->disconnect_by = strdup(dst_addr);
                    }
                }
            } else if (reqresp == 181 || reqresp == 302 || reqresp == 301) {
                // Bob has diversion
                call->state = SIP_CALLSTATE_DIVERTED;
                // Don't store disconnect info for 181 - wait for final response
            } else if ((reqresp == 480 || reqresp == 404 || reqresp == 503 || reqresp == 488 || reqresp == 603) && 
                       (call->state == SIP_CALLSTATE_DIVERTED || call->state == SIP_CALLSTATE_CALLSETUP)) {
                // After diversion or during setup, got error response
                // 480=Temporarily Unavailable, 404=Not Found, 503=Service Unavailable, 488=Not Acceptable, 603=Decline
                if (!call->disconnect_code) {
                    const char *resp_str = sip_get_msg_reqresp_str(msg);
                    if (resp_str) {
                        call->disconnect_code = strdup(resp_str);
                    } else {
                        char code_str[32];
                        sprintf(code_str, "%d", reqresp);
                        call->disconnect_code = strdup(code_str);
                    }
                }
                // Store source IP (who sent the error)
                if (!call->disconnect_by) {
                    char addr[256];
                    sprintf(addr, "%s:%u", msg->packet->src.ip, msg->packet->src.port);
                    call->disconnect_by = strdup(addr);
                }
                // Update state based on response - keep DIVERTED if already diverted
                if (call->state != SIP_CALLSTATE_DIVERTED) {
                    if (reqresp == 480 || reqresp == 503) {
                        call->state = SIP_CALLSTATE_BUSY;
                    } else {
                        call->state = SIP_CALLSTATE_REJECTED;
                    }
                }
                // For DIVERTED calls, keep the state as DIVERTED even with 480
            }
        } else if (call->state == SIP_CALLSTATE_CANCELLED && reqresp == 487) {
            // 487 Request Terminated after CANCEL - update the disconnect code
            if (call->disconnect_code && strcmp(call->disconnect_code, "CANCELLED") == 0) {
                // Update from generic "CANCELLED" to specific "487 Request Terminated"
                sng_free(call->disconnect_code);
                const char *resp_str = sip_get_msg_reqresp_str(msg);
                if (resp_str) {
                    call->disconnect_code = strdup(resp_str);
                } else {
                    call->disconnect_code = strdup("487 Request Terminated");
                }
            }
        } else if (call->state == SIP_CALLSTATE_INCALL) {
            // Handle messages during active call
            // BYE is already handled above, so skip it here
            if (reqresp == 603) {
                // Call declined during active call (unusual but possible)
                call->state = SIP_CALLSTATE_REJECTED;
                if (!call->disconnect_code) {
                    const char *resp_str = sip_get_msg_reqresp_str(msg);
                    if (resp_str) {
                        call->disconnect_code = strdup(resp_str);
                    } else {
                        call->disconnect_code = strdup("603 Decline");
                    }
                }
                if (!call->disconnect_by) {
                    char src_addr[256];
                    sprintf(src_addr, "%s:%u", msg->packet->src.ip, msg->packet->src.port);
                    call->disconnect_by = strdup(src_addr);
                }
            } else if (reqresp >= 200 && reqresp < 700 && msg->cseq > 0) {
                // Check if this is a response to a BYE
                sip_msg_t *bye_msg = NULL;
                int i;
                for (i = 0; i < call_msg_count(call); i++) {
                    sip_msg_t *m = vector_item(call->msgs, i);
                    if (m && m->reqresp == SIP_METHOD_BYE && m->cseq == msg->cseq) {
                        bye_msg = m;
                        break;
                    }
                }
                if (bye_msg) {
                    // BYE response received - ALWAYS mark call as completed
                    call->state = SIP_CALLSTATE_COMPLETED;
                    
                    // Update disconnect code from BYE to the response
                    if (call->disconnect_code && strcmp(call->disconnect_code, "BYE") == 0) {
                        sng_free(call->disconnect_code);
                        call->disconnect_code = NULL;
                    }
                    if (!call->disconnect_code) {
                        // Store the response code for the BYE
                        const char *resp_str = sip_get_msg_reqresp_str(msg);
                        if (resp_str) {
                            call->disconnect_code = strdup(resp_str);
                        } else {
                            char code_str[32];
                            sprintf(code_str, "%d", reqresp);
                            call->disconnect_code = strdup(code_str);
                        }
                    }
                    // Store who confirmed the BYE if not already set
                    if (!call->disconnect_by) {
                        char addr[256];
                        sprintf(addr, "%s:%u", msg->packet->src.ip, msg->packet->src.port);
                        call->disconnect_by = strdup(addr);
                    }
                }
            }
        } else if (reqresp == SIP_METHOD_INVITE && call->state !=  SIP_CALLSTATE_INCALL) {
            // Call is being setup (after proper authentication)
            call->invitecseq = msg->cseq;
            call->state = SIP_CALLSTATE_CALLSETUP;
        }
    } else if (reqresp != SIP_METHOD_BYE) {
        // This is actually a call (but not a BYE which was already handled)
        if (reqresp == SIP_METHOD_INVITE) {
            call->invitecseq = msg->cseq;
            call->state = SIP_CALLSTATE_CALLSETUP;
        }
    }
}

const char *
call_get_attribute(sip_call_t *call, enum sip_attr_id id, char *value)
{
    sip_msg_t *first, *last;

    if (!call)
        return NULL;

    switch (id) {
        case SIP_ATTR_CALLINDEX:
            sprintf(value, "%d", call->index);
            break;
        case SIP_ATTR_CALLID:
            sprintf(value, "%s", call->callid);
            break;
        case SIP_ATTR_XCALLID:
            sprintf(value, "%s", call->xcallid);
            break;
        case SIP_ATTR_MSGCNT:
            sprintf(value, "%d", vector_count(call->msgs));
            break;
        case SIP_ATTR_CALLSTATE:
            sprintf(value, "%s", call_state_to_str(call->state));
            break;
        case SIP_ATTR_TRANSPORT:
            first = vector_first(call->msgs);
            sprintf(value, "%s", sip_transport_str(first->packet->type));
            break;
        case SIP_ATTR_CONVDUR:
            timeval_to_duration(msg_get_time(call->cstart_msg), msg_get_time(call->cend_msg), value);
            break;
        case SIP_ATTR_TOTALDUR:
            first = vector_first(call->msgs);
            last = vector_last(call->msgs);
            timeval_to_duration(msg_get_time(first), msg_get_time(last), value);
            break;
        case SIP_ATTR_REASON_TXT:
            if (call->reasontxt)
                sprintf(value, "%s", call->reasontxt);
            break;
        case SIP_ATTR_WARNING:
            if (call->warning)
                sprintf(value, "%d", call->warning);
            break;
        case SIP_ATTR_DISCONNECT_BY:
            // Don't show disconnect info for calls still in setup
            if (call->state == SIP_CALLSTATE_CALLSETUP) {
                sprintf(value, "-");
            } else if (call->disconnect_by) {
                // Already stored as IP:port, just copy it
                sprintf(value, "%s", call->disconnect_by);
            } else if (call->state == SIP_CALLSTATE_CANCELLED || 
                       call->state == SIP_CALLSTATE_REJECTED ||
                       call->state == SIP_CALLSTATE_BUSY ||
                       call->state == SIP_CALLSTATE_COMPLETED ||
                       call->state == SIP_CALLSTATE_DIVERTED ||
                       call->state == SIP_CALLSTATE_INCALL) {
                // Fallback: Find who terminated the call
                sip_msg_t *term_msg = NULL;
                int i;
                
                // Find termination message (CANCEL, BYE, or final error response)
                for (i = call_msg_count(call) - 1; i >= 0; i--) {
                    sip_msg_t *m = vector_item(call->msgs, i);
                    if (!m || !m->packet) continue;
                    
                    // Look for termination messages
                    if (m->reqresp == SIP_METHOD_CANCEL || 
                        m->reqresp == SIP_METHOD_BYE) {
                        term_msg = m;
                        break;
                    }
                    // Look for final error responses (skip auth challenges)
                    if (m->reqresp >= 400 && m->reqresp < 700 && 
                        m->reqresp != 401 && m->reqresp != 407) {
                        term_msg = m;
                        break;
                    }
                }
                
                if (term_msg && term_msg->packet) {
                    // Show source IP:port of termination message
                    sprintf(value, "%s:%u", term_msg->packet->src.ip, term_msg->packet->src.port);
                } else if (call->state == SIP_CALLSTATE_INCALL) {
                    // Call is still active, no disconnect yet
                    sprintf(value, "-");
                } else {
                    // No termination found for ended call
                    sprintf(value, "Unknown");
                }
            }
            break;
        case SIP_ATTR_DISCONNECT_CODE:
            // Don't show disconnect code for calls still in setup
            if (call->state == SIP_CALLSTATE_CALLSETUP) {
                sprintf(value, "-");
            } else if (call->disconnect_code) {
                sprintf(value, "%s", call->disconnect_code);
            } else if (call->state == SIP_CALLSTATE_INCALL) {
                // Check if there's a BYE without response (timeout/lost)
                int i;
                sip_msg_t *bye_found = NULL;
                for (i = call_msg_count(call) - 1; i >= 0; i--) {
                    sip_msg_t *m = vector_item(call->msgs, i);
                    if (m && m->reqresp == SIP_METHOD_BYE) {
                        bye_found = m;
                        sprintf(value, "BYE (No Response)");
                        break;
                    }
                }
                // If no BYE found, call is still active
                if (!bye_found) {
                    sprintf(value, "-");
                }
            } else if (call->state == SIP_CALLSTATE_CANCELLED) {
                // Look for 487 response
                int i;
                for (i = 0; i < call_msg_count(call); i++) {
                    sip_msg_t *m = vector_item(call->msgs, i);
                    if (m && m->reqresp == 487) {
                        sprintf(value, "487 Request Terminated");
                        break;
                    }
                }
                if (!strlen(value)) {
                    sprintf(value, "CANCELLED");
                }
            } else if (call->state == SIP_CALLSTATE_DIVERTED) {
                // Look for error response after diversion (480, 404, 503, etc.)
                int i;
                for (i = call_msg_count(call) - 1; i >= 0; i--) {
                    sip_msg_t *m = vector_item(call->msgs, i);
                    if (m && m->reqresp >= 400 && m->reqresp < 700 && 
                        m->reqresp != 401 && m->reqresp != 407) {
                        const char *resp_str = sip_get_msg_reqresp_str(m);
                        if (resp_str) {
                            sprintf(value, "%s", resp_str);
                        } else {
                            sprintf(value, "%d", m->reqresp);
                        }
                        break;
                    }
                }
                if (!strlen(value)) {
                    sprintf(value, "DIVERTED");
                }
            } else if (call->state == SIP_CALLSTATE_REJECTED) {
                sprintf(value, "REJECTED");
            } else if (call->state == SIP_CALLSTATE_BUSY) {
                sprintf(value, "BUSY");
            } else if (call->state == SIP_CALLSTATE_COMPLETED) {
                sprintf(value, "BYE");
            }
            break;
        default:
            return msg_get_attribute(vector_first(call->msgs), id, value);
            break;
    }

    return strlen(value) ? value : NULL;
}

const char *
call_state_to_str(int state)
{
    switch (state) {
        case SIP_CALLSTATE_CALLSETUP:
            return "CALL SETUP";
        case SIP_CALLSTATE_INCALL:
            return "IN CALL";
        case SIP_CALLSTATE_CANCELLED:
            return "CANCELLED";
        case SIP_CALLSTATE_REJECTED:
            return "REJECTED";
        case SIP_CALLSTATE_BUSY:
            return "BUSY";
        case SIP_CALLSTATE_DIVERTED:
            return "DIVERTED";
        case SIP_CALLSTATE_COMPLETED:
            return "COMPLETED";
    }
    return "";
}

int
call_attr_compare(sip_call_t *one, sip_call_t *two, enum sip_attr_id id)
{
    char onevalue[256], twovalue[256];
    int oneintvalue, twointvalue;
    int comparetype; /* TODO 0 = string compare, 1 = int comprare */

    switch (id) {
        case SIP_ATTR_CALLINDEX:
            oneintvalue = one->index;
            twointvalue = two->index;
            comparetype = 1;
            break;
        case SIP_ATTR_MSGCNT:
            oneintvalue = call_msg_count(one);
            twointvalue = call_msg_count(two);
            comparetype = 1;
            break;
        default:
            // Get attribute values
            memset(onevalue, 0, sizeof(onevalue));
            memset(twovalue, 0, sizeof(twovalue));
            call_get_attribute(one, id, onevalue);
            call_get_attribute(two, id, twovalue);
            comparetype = 0;
            break;
    }

    switch (comparetype) {
        case 0:
            if (strlen(twovalue) == 0 && strlen(onevalue) == 0)
                return 0;
            if (strlen(twovalue) == 0)
                return 1;
            if (strlen(onevalue) == 0)
                return -1;
            return strcmp(onevalue, twovalue);
        case 1:
            if (oneintvalue == twointvalue) return 0;
            if (oneintvalue > twointvalue) return 1;
            if (oneintvalue < twointvalue) return -1;
            /* no break */
        default:
            return 0;
    }
}

void
call_add_xcall(sip_call_t *call, sip_call_t *xcall)
{
    if (!call || !xcall)
        return;

    // Mark this call as changed
    call->changed = true;
    // Add the xcall to the list
    vector_append(call->xcalls, xcall);
}
