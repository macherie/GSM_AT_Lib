/**
 * \file            gsm_parser.c
 * \brief           Parse incoming data from AT port
 */

/*
 * Copyright (c) 2018 Tilen Majerle
 *  
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software, 
 * and to permit persons to whom the Software is furnished to do so, 
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
 * AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * This file is part of GSM-AT.
 *
 * Author:          Tilen MAJERLE <tilen@majerle.eu>
 */
#include "gsm/gsm_private.h"
#include "gsm/gsm_parser.h"
#include "gsm/gsm_mem.h"

/**
 * \brief           Parse number from string
 * \note            Input string pointer is changed and number is skipped
 * \param[in]       Pointer to pointer to string to parse
 * \return          Parsed number
 */
int32_t
gsmi_parse_number(const char** str) {
    int32_t val = 0;
    uint8_t minus = 0;
    const char* p = *str;                       /*  */
    
    if (*p == '"') {                            /* Skip leading quotes */
        p++;
    }
    if (*p == ',') {                            /* Skip leading comma */
        p++;
    }
    if (*p == '"') {                            /* Skip leading quotes */
        p++;
    }
    if (*p == '/') {                            /* Skip '/' character, used in datetime */
        p++;
    }
    if (*p == ':') {                            /* Skip ':' character, used in datetime */
        p++;
    }
    if (*p == '+') {                            /* Skip '+' character, used in datetime */
        p++;
    }
    if (*p == '-') {                            /* Check negative number */
        minus = 1;
        p++;
    }
    while (GSM_CHARISNUM(*p)) {                 /* Parse until character is valid number */
        val = val * 10 + GSM_CHARTONUM(*p);
        p++;
    }
    if (*p == ',') {                            /* Go to next entry if possible */
        p++;
    }
    *str = p;                                   /* Save new pointer with new offset */
    
    return minus ? -val : val;
}

/**
 * \brief           Parse number from string as hex
 * \note            Input string pointer is changed and number is skipped
 * \param[in]       Pointer to pointer to string to parse
 * \return          Parsed number
 */
uint32_t
gsmi_parse_hexnumber(const char** str) {
    int32_t val = 0;
    const char* p = *str;                       /*  */
    
    if (*p == '"') {                            /* Skip leading quotes */
        p++;
    }
    if (*p == ',') {                            /* Skip leading comma */
        p++;
    }
    if (*p == '"') {                            /* Skip leading quotes */
        p++;
    }
    while (GSM_CHARISHEXNUM(*p)) {              /* Parse until character is valid number */
        val = val * 16 + GSM_CHARHEXTONUM(*p);
        p++;
    }
    if (*p == ',') {                            /* Go to next entry if possible */
        p++;
    }
    *str = p;                                   /* Save new pointer with new offset */
    return val;
}

/**
 * \brief           Parse input string as string part of AT command
 * \param[in,out]   src: Pointer to pointer to string to parse from
 * \param[in]       dst: Destination pointer. Use NULL in case you want only skip string in source
 * \param[in]       dst_len: Length of distance buffer, including memory for NULL termination
 * \param[in]       trim: Set to 1 to process entire string, even if no memory anymore
 * \return          1 on success, 0 otherwise
 */
uint8_t
gsmi_parse_string(const char** src, char* dst, size_t dst_len, uint8_t trim) {
    const char* p = *src;
    size_t i;
    
    if (*p == ',') {
        p++;
    }
    if (*p == '"') {
        p++;
    }
    i = 0;
    if (dst_len) {
        dst_len--;
    }
    while (*p) {
        if (*p == '"' && (p[1] == ',' || p[1] == '\r' || p[1] == '\n')) {
            p++;
            break;
        }
        if (dst != NULL) {
            if (i < dst_len) {
                *dst++ = *p;
                i++;
            } else if (!trim) {
                break;
            }
        }
        p++;
    }
    if (dst != NULL) {
        *dst = 0;
    }
    *src = p;
    return 1;
}

/**
 * \brief           Check current string position and trim to the next entry
 * \param[in]       src: Pointer to pointer to input string
 */
void
gsmi_check_and_trim(const char** src) {
    const char* t = *src;
    if (*t != '"' && *t != '\r' && *t != ',') { /* Check if trim required */
        gsmi_parse_string(src, NULL, 0, 1);     /* Trim to the end */
    }
}

/**
 * \brief           Parse string as IP address
 * \param[in,out]   src: Pointer to pointer to string to parse from
 * \param[in]       dst: Destination pointer
 * \return          1 on success, 0 otherwise
 */
uint8_t
gsmi_parse_ip(const char** src, gsm_ip_t* ip) {
    const char* p = *src;
    
    if (*p == '"') {
        p++;
    }
    ip->ip[0] = gsmi_parse_number(&p); p++;
    ip->ip[1] = gsmi_parse_number(&p); p++;
    ip->ip[2] = gsmi_parse_number(&p); p++;
    ip->ip[3] = gsmi_parse_number(&p);
    if (*p == '"') {
        p++;
    }
    
    *src = p;                                   /* Set new pointer */
    return 1;
}

/**
 * \brief           Parse string as MAC address
 * \param[in,out]   src: Pointer to pointer to string to parse from
 * \param[in]       dst: Destination pointer
 * \return          1 on success, 0 otherwise
 */
uint8_t
gsmi_parse_mac(const char** src, gsm_mac_t* mac) {
    const char* p = *src;
    
    if (*p == '"') {
        p++;
    }
    mac->mac[0] = gsmi_parse_hexnumber(&p); p++;
    mac->mac[1] = gsmi_parse_hexnumber(&p); p++;
    mac->mac[2] = gsmi_parse_hexnumber(&p); p++;
    mac->mac[3] = gsmi_parse_hexnumber(&p); p++;
    mac->mac[4] = gsmi_parse_hexnumber(&p); p++;
    mac->mac[5] = gsmi_parse_hexnumber(&p);
    if (*p == '"') {
        p++;
    }
    if (*p == ',') {
        p++;
    }
    *src = p;
    return 1;
}

/**
 * \brief           Parse memory string, ex. "SM", "ME", "MT", etc
 * \param[in,out]   src: Pointer to pointer to string to parse from
 * \return          Parsed memory
 */
gsm_mem_t
gsmi_parse_memory(const char** src) {
    size_t i, sl;
    gsm_mem_t mem = GSM_MEM_UNKNOWN;
    const char* s = *src;

    if (*s == ',') {
        s++;
    }
    if (*s == '"') {
        s++;
    }

    /* Scan all memories available for modem */
    for (i = 0; i < gsm_dev_mem_map_size; i++) {
        sl = strlen(gsm_dev_mem_map[i].mem_str);
        if (!strncmp(s, gsm_dev_mem_map[i].mem_str, sl)) {
            mem = gsm_dev_mem_map[i].mem;
            s += sl;
            break;
        }
    }

    if (mem == GSM_MEM_UNKNOWN) {
        gsmi_parse_string(&s, NULL, 0, 1);      /* Skip string */
    }
    if (*s == '"') {
        s++;
    }
    *src = s;
    return mem;
}


/**
 * \brief           Parse a string of memories in format "M1","M2","M3","M4",...
 * \param[in,out]   src: Pointer to pointer to string to parse from
 * \param[out]      mem_dst: Output result with memory list as bit field
 * \return          1 on success, 0 otherwise
 */
uint8_t
gsmi_parse_memories_string(const char** src, uint32_t* mem_dst) {
    const char* str = *src;
    gsm_mem_t mem;

    *mem_dst = 0;
    if (*str == ',') {
        str++;
    }
    if (*str == '(') {
        str++;
    }
    do {
        mem = gsmi_parse_memory(&str);          /* Parse memory string */
        *mem_dst |= GSM_U32(1 << GSM_U32(mem)); /* Set as bit field */
    } while (*str && *str != ')');
    if (*str == ')') {
        str++;
    }
    *src = str;
    return 1;
}

/**
 * \brief           Parse received +CREG message
 * \param[in]       str: Input string
 * \return          1 on success, 0 otherwise
 */
uint8_t
gsmi_parse_creg(const char* str, uint8_t skip_first) {
    uint8_t cb = 0;
    if (*str == '+') {
        str += 7;
    }

    if (skip_first) {
        gsmi_parse_number(&str);
    }
    gsm.network.status = (gsm_network_reg_status_t)gsmi_parse_number(&str);

    /*
     * In case we are connected to network,
     * scan for current network info
     */
    if (gsm.network.status == GSM_NETWORK_REG_STATUS_CONNECTED ||
        gsm.network.status == GSM_NETWORK_REG_STATUS_CONNECTED_ROAMING) {
        if (gsm_operator_get(0) != gsmOK) {     /* Notify user in case we are not able to add new command to queue */
            cb = 1;
        }
    } else {
        cb = 1;
    }

    /**
     * \todo: Process callback
     */
    if (cb) {                                   /* Check for callback */

    }

    return 1;
}

/**
 * \brief           Parse received +CPIN status value
 * \param[in]       str: Input string
 * \param[in]       send_evt: Send event about new CPIN status
 * \return          1 on success, 0 otherwise
 */
uint8_t
gsmi_parse_cpin(const char* str, uint8_t send_evt) {
    if (*str == '+') {
        str += 7;
    }
    if (!strncmp(str, "READY", 5)) {
        gsm.sim_state = GSM_SIM_STATE_READY;
    } else if (!strncmp(str, "NOT READY", 9)) {
        gsm.sim_state = GSM_SIM_STATE_NOT_READY;
    } else if (!strncmp(str, "NOT INSERTED", 14)) {
        gsm.sim_state = GSM_SIM_STATE_NOT_INSERTED;
    } else if (!strncmp(str, "SIM PIN", 7)) {
        gsm.sim_state = GSM_SIM_STATE_PIN;
    } else if (!strncmp(str, "PIN PUK", 7)) {
        gsm.sim_state = GSM_SIM_STATE_PUK;
    } else {
        gsm.sim_state = GSM_SIM_STATE_NOT_READY;
    }

    /*
     * In case SIM is ready,
     * start with basic info about SIM
     */
    if (gsm.sim_state == GSM_SIM_STATE_READY) {
        gsmi_get_sim_info(0);
    }

    if (send_evt) {
        gsm.cb.cb.cpin.state = gsm.sim_state;
        gsmi_send_cb(GSM_CB_CPIN);              /* SIM card event */
    }
    return 1;
}

/**
 * \brief           Parse +COPS string from COPS? command
 * \param[in]       str: Input string
 * \return          1 on success, 0 otherwise
 */
uint8_t
gsmi_parse_cops(const char* str) {
    if (*str == '+') {
        str += 7;
    }

    gsm.network.curr_operator.mode = (gsm_operator_mode_t)gsmi_parse_number(&str);
    if (*str != '\r') {
        gsm.network.curr_operator.format = (gsm_operator_format_t)gsmi_parse_number(&str);
        if (*str != '\r') {
            switch (gsm.network.curr_operator.format) {
                case GSM_OPERATOR_FORMAT_LONG_NAME:
                    gsmi_parse_string(&str, gsm.network.curr_operator.data.long_name, sizeof(gsm.network.curr_operator.data.long_name), 1);
                    break;
                case GSM_OPERATOR_FORMAT_SHORT_NAME:
                    gsmi_parse_string(&str, gsm.network.curr_operator.data.short_name, sizeof(gsm.network.curr_operator.data.short_name), 1);
                    break;
                case GSM_OPERATOR_FORMAT_NUMBER:
                    gsm.network.curr_operator.data.num = GSM_U32(gsmi_parse_number(&str, 1));
                    break;
            }
        }
    } else {
        gsm.network.curr_operator.format = GSM_OPERATOR_FORMAT_INVALID;
    }

    if (gsm.msg != NULL && gsm.msg->cmd_def == GSM_CMD_COPS_GET &&
        gsm.msg->msg.cops_get.curr != NULL) {   /* Check and copy to user variable */
        memcpy(gsm.msg->msg.cops_get.curr, &gsm.network.curr_operator, sizeof(*gsm.msg->msg.cops_get.curr));
    }
    return 1;
}

/**
 * \brief           Parse +COPS received statement byte by byte
 * \note            Command must be active and message set to use this function
 * \param[in]       ch: New character to parse
 * \param[in]       reset: Flag to reset state machine
 * \return          1 on success, 0 otherwise
 */
uint8_t
gsmi_parse_cops_scan(uint8_t ch, uint8_t reset) {
    static union {
        struct {
            uint8_t bo:1;                       /*!< Bracket open flag (Bracket Open) */
            uint8_t ccd:1;                      /*!< 2 consecutive commas detected in a row (Comma Comma Detected) */
            uint8_t tn:2;                       /*!< Term number in response, 2 bits for 4 diff values */
            uint8_t tp;                         /*!< Current term character position */
            uint8_t ch_prev;                    /*!< Previous character */
        } f;
    } u;

    if (reset) {                                /* Check for reset status */
        memset(&u, 0x00, sizeof(u));            /* Reset everything */
        u.f.ch_prev = 0;
        return 1;
    }

    if (!u.f.ch_prev) {                         /* Check if this is first character */
        if (ch == ' ') {                        /* Skip leading spaces */
            return 1;
        } else if (ch == ',') {                 /* If first character is comma, no operators available */
            u.f.ccd = 1;                        /* Fake double commas in a row */
        }
    }
 
    if (u.f.ccd ||                              /* Ignore data after 2 commas in a row */
        gsm.msg->msg.cops_scan.opsi >= gsm.msg->msg.cops_scan.opsl) {   /* or if array is full */
        return 1;
    }

    if (u.f.bo) {                               /* Bracket already open */
        if (ch == ')') {                        /* Close bracket check */
            u.f.bo = 0;                         /* Clear bracket open flag */
            u.f.tn = 0;                         /* Go to next term */
            u.f.tp = 0;                         /* Go to beginning of next term */
            gsm.msg->msg.cops_scan.opsi++;      /* Increase index */
            if (gsm.msg->msg.cops_scan.opf != NULL) {
                *gsm.msg->msg.cops_scan.opf = gsm.msg->msg.cops_scan.opsi;
            }
        } else if (ch == ',') {
            u.f.tn++;                           /* Go to next term */
            u.f.tp = 0;                         /* Go to beginning of next term */
        } else if (ch != '"') {                 /* We have valid data */
            size_t i = gsm.msg->msg.cops_scan.opsi;
            switch (u.f.tn) {
                case 0: {                       /* Parse status info */
                    gsm.msg->msg.cops_scan.ops[i].stat = (gsm_operator_status_t)(10 * (size_t)gsm.msg->msg.cops_scan.ops[i].stat + (ch - '0'));
                    break;
                }
                case 1: {                       /*!< Parse long name */
                    if (u.f.tp < sizeof(gsm.msg->msg.cops_scan.ops[i].long_name) - 1) {
                        gsm.msg->msg.cops_scan.ops[i].long_name[u.f.tp++] = ch;
                        gsm.msg->msg.cops_scan.ops[i].long_name[u.f.tp] = 0;
                    }
                    break;
                }
                case 2: {                       /*!< Parse short name */
                    if (u.f.tp < sizeof(gsm.msg->msg.cops_scan.ops[i].short_name) - 1) {
                        gsm.msg->msg.cops_scan.ops[i].short_name[u.f.tp++] = ch;
                        gsm.msg->msg.cops_scan.ops[i].short_name[u.f.tp] = 0;
                    }
                    break;
                }
                case 3: {                       /*!< Parse number */
                    gsm.msg->msg.cops_scan.ops[i].num = (10 * gsm.msg->msg.cops_scan.ops[i].num) + (ch - '0');
                    break;
                }
                default: break;
            }
        }
    } else {
        if (ch == '(') {                        /* Check for opening bracket */
            u.f.bo = 1;
        } else if (ch == ',' && u.f.ch_prev == ',') {
            u.f.ccd = 1;                        /* 2 commas in a row */
        }
    }
    u.f.ch_prev = ch;
    return 1;
}

/**
 * \brief           Parse datetime in format dd/mm/yy,hh:mm:ss
 * \param[in]       src: Pointer to pointer to input string
 * \param[out]      dt: Date time structure
 * \return          1 on success, 0 otherwise
 */
uint8_t
gsmi_parse_datetime(const char** src, gsm_datetime_t* dt) {
    dt->date = gsmi_parse_number(src);
    dt->month = gsmi_parse_number(src);
    dt->year = GSM_U16(2000) + gsmi_parse_number(src);
    dt->hours = gsmi_parse_number(src);
    dt->minutes = gsmi_parse_number(src);
    dt->seconds = gsmi_parse_number(src);

    gsmi_check_and_trim(src);                   /* Trim text to the end */
    return 1;
}

#if GSM_CFG_CALL || __DOXYGEN__

/**
 * \brief           Parse received +CLCC with call status info
 * \param[in]       str: Input string
 * \param[in]       send_evt: Send event about new CPIN status
 * \return          1 on success, 0 otherwise
 */
uint8_t
gsmi_parse_clcc(const char* str, uint8_t send_evt) {
    if (*str == '+') {
        str += 7;
    }

    gsm.call.id = gsmi_parse_number(&str);
    gsm.call.dir = (gsm_call_dir_t)gsmi_parse_number(&str);
    gsm.call.state = (gsm_call_state_t)gsmi_parse_number(&str);
    gsm.call.type = (gsm_call_type_t)gsmi_parse_number(&str);
    gsm.call.is_multipart = (gsm_call_type_t)gsmi_parse_number(&str);
    gsmi_parse_string(&str, gsm.call.number, sizeof(gsm.call.number), 1);
    gsm.call.addr_type = gsmi_parse_number(&str);
    gsmi_parse_string(&str, gsm.call.name, sizeof(gsm.call.name), 1);

    if (send_evt) {
        gsm.cb.cb.call_changed.call = &gsm.call;
        gsmi_send_cb(GSM_CB_CALL_CHANGED);
    }
    return 1;
}

#endif /* GSM_CFG_CALL || __DOXYGEN__ */

#if GSM_CFG_SMS || __DOXYGEN__

/**
 * \brief           Parse string and check for type of SMS state
 * \param[in]       src: Pointer to pointer to string to parse
 * \param[out]      stat: Output status variable
 * \return          1 on success, 0 otherwise
 */
uint8_t
gsmi_parse_sms_status(const char** src, gsm_sms_status_t* stat) {
    gsm_sms_status_t s;
    char t[11];

    gsmi_parse_string(src, t, sizeof(t), 1);    /* Parse string and advance */
    if (!strcmp(t, "REC UNREAD")) {
        s = GSM_SMS_STATUS_UNREAD;
    } else if (!strcmp(t, "REC READ")) {
        s = GSM_SMS_STATUS_READ;
    } else if (!strcmp(t, "STO UNSENT")) {
        s = GSM_SMS_STATUS_UNSENT;
    } else if (!strcmp(t, "REC SENT")) {
        s = GSM_SMS_STATUS_SENT;
    } else {
        s = GSM_SMS_STATUS_ALL;                 /* Error! */
    }
    if (s != GSM_SMS_STATUS_ALL) {
        *stat = s;
        return 1;
    }
    return 0;
}

/**
 * \brief           Parse received +CMGS with last sent SMS memory info
 * \param[in]       str: Input string
 * \param[in]       send_evt: Send event to user
 * \return          1 on success, 0 otherwise
 */
uint8_t
gsmi_parse_cmgs(const char* str, uint8_t send_evt) {
    uint16_t num;
    if (*str == '+') {
        str += 7;
    }

    num = gsmi_parse_number(&str);              /* Parse number */

    if (send_evt) {
        gsm.cb.cb.sms_sent.num = num;
        gsmi_send_cb(GSM_CB_SMS_SENT);          /* SIM card event */
    }
    return 1;
}

/**
 * \brief           Parse +CMGR statement
 * \todo            Parse date and time from SMS entry
 * \param[in]       str: Input string
 * \return          1 on success, 0 otherwise
 */
uint8_t
gsmi_parse_cmgr(const char* str) {
    gsm_sms_entry_t* e;
    if (*str == '+') {
        str += 7;
    }
    
    e = gsm.msg->msg.sms_read.entry;
    gsmi_parse_sms_status(&str, &e->status);
    gsmi_parse_string(&str, e->number, sizeof(e->number), 1);
    gsmi_parse_string(&str, e->name, sizeof(e->name), 1);
    gsmi_parse_datetime(&str, &e->datetime);

    return 1;
}

/**
 * \brief           Parse +CMGL statement
 * \todo            Parse date and time from SMS entry
 * \param[in]       str: Input string
 * \return          1 on success, 0 otherwise
 */
uint8_t
gsmi_parse_cmgl(const char* str) {
    gsm_sms_entry_t* e;

    if (gsm.msg == NULL || gsm.msg->cmd_def != GSM_CMD_CMGL ||
        gsm.msg->msg.sms_list.ei >= gsm.msg->msg.sms_list.etr) {
        return 0;
    }

    if (*str == '+') {
        str += 7;
    }

    e = &gsm.msg->msg.sms_list.entries[gsm.msg->msg.sms_list.ei];
    e->mem = gsm.msg->msg.sms_list.mem;         /* Manually set memory */
    e->pos = GSM_SZ(gsmi_parse_number(&str));   /* Scan position */
    gsmi_parse_sms_status(&str, &e->status);
    gsmi_parse_string(&str, e->number, sizeof(e->number), 1);
    gsmi_parse_string(&str, e->name, sizeof(e->name), 1);
    gsmi_parse_datetime(&str, &e->datetime);

    return 1;
}

/**
 * \brief           Parse received +CMTI with received SMS info
 * \param[in]       str: Input string
 * \param[in]       send_evt: Send event about new CPIN status
 * \return          1 on success, 0 otherwise
 */
uint8_t
gsmi_parse_cmti(const char* str, uint8_t send_evt) {
    if (*str == '+') {
        str += 7;
    }

    gsm.cb.cb.sms_recv.mem = gsmi_parse_memory(&str);   /* Parse memory string */
    gsm.cb.cb.sms_recv.pos = gsmi_parse_number(&str);   /* Parse number */

    if (send_evt) {
        gsmi_send_cb(GSM_CB_SMS_RECV);          /* SIM card event */
    }
    return 1;
}

/**
 * \brief           Parse +CPMS statement
 * \param[in]       str: Input string
 * \param[in]       opt: Expected input: 0 = CPMS_OPT, 1 = CPMS_GET, 2 = CPMS_SET
 * \return          1 on success, 0 otherwise
 */
uint8_t
gsmi_parse_cpms(const char* str, uint8_t opt) {
    uint8_t i;
    if (*str == '+') {
        str += 7;
    }
    switch (opt) {                              /* Check expected input string */
        case 0: {                               /* Get list of CPMS options: +CPMS: (("","","",..),("....")("...")) */
            for (i = 0; i < 3; i++) {           /* 3 different memories for "operation","receive","sent" */
                if (!gsmi_parse_memories_string(&str, &gsm.sms.mem[i].mem_available)) {
                    return 0;
                }
            }
            break;
        }
        case 1: {                               /* Received statement of current info: +CPMS: "ME",10,20,"SE",2,20,"... */
            for (i = 0; i < 3; i++) {           /* 3 memories expected */
                gsm.sms.mem[i].current = gsmi_parse_memory(&str);   /* Parse memory string and save it as current */
                gsm.sms.mem[i].used = gsmi_parse_number(&str);  /* Get used memory size */
                gsm.sms.mem[i].total = gsmi_parse_number(&str); /* Get total memory size */
            }
            break;
        }
        case 2: {                               /* Received statement of set info: +CPMS: 10,20,2,20 */
            for (i = 0; i < 3; i++) {           /* 3 memories expected */
                gsm.sms.mem[i].used = gsmi_parse_number(&str);  /* Get used memory size */
                gsm.sms.mem[i].total = gsmi_parse_number(&str); /* Get total memory size */
            }
            break;
        }
        default: break;
    }
    return 1;
}

#endif /* GSM_CFG_SMS || __DOXYGEN__ */

#if GSM_CFG_PHONEBOOK || __DOXYGEN__

/**
 * \brief           Parse +CPBS statement
 * \param[in]       str: Input string
 * \param[in]       opt: Expected input: 0 = CPBS_OPT, 1 = CPBS_GET, 2 = CPBS_SET
 * \return          1 on success, 0 otherwise
 */
uint8_t
gsmi_parse_cpbs(const char* str, uint8_t opt) {
    if (*str == '+') {
        str += 7;
    }
    switch (opt) {                              /* Check expected input string */
        case 0: {                               /* Get list of CPBS options: ("M1","M2","M3",...) */
            return gsmi_parse_memories_string(&str, &gsm.pb.mem.mem_available);
        }
        case 1: {                               /* Received statement of current info: +CPBS: "ME",10,20 */
            gsm.pb.mem.current = gsmi_parse_memory(&str);   /* Parse memory string and save it as current */
            gsm.pb.mem.used = gsmi_parse_number(&str);  /* Get used memory size */
            gsm.pb.mem.total = gsmi_parse_number(&str); /* Get total memory size */
            break;
        }
        case 2: {                               /* Received statement of set info: +CPBS: 10,20 */
            gsm.pb.mem.used = gsmi_parse_number(&str);  /* Get used memory size */
            gsm.pb.mem.total = gsmi_parse_number(&str); /* Get total memory size */
            break;
        }
    }
    return 1;
}

/**
 * \brief           Parse +CPBR statement
 * \param[in]       str: Input string
 * \return          1 on success, 0 otherwise
 */
uint8_t
gsmi_parse_cpbr(const char* str) {
    gsm_pb_entry_t* e;

    if (gsm.msg == NULL || gsm.msg->cmd_def != GSM_CMD_CPBR ||
        gsm.msg->msg.pb_list.ei >= gsm.msg->msg.pb_list.etr) {
        return 0;
    }

    if (*str == '+') {
        str += 7;
    }
    
    e = &gsm.msg->msg.pb_list.entries[gsm.msg->msg.pb_list.ei];
    e->pos = GSM_SZ(gsmi_parse_number(&str));
    gsmi_parse_string(&str, e->name, sizeof(e->name), 1);
    e->type = (gsm_number_type_t)gsmi_parse_number(&str);
    gsmi_parse_string(&str, e->number, sizeof(e->number), 1);

    gsm.msg->msg.pb_list.ei++;
    if (gsm.msg->msg.pb_list.er != NULL) {
        *gsm.msg->msg.pb_list.er = gsm.msg->msg.pb_list.ei;
    }
    return 1;
}

/**
 * \brief           Parse +CPBF statement
 * \param[in]       str: Input string
 * \return          1 on success, 0 otherwise
 */
uint8_t
gsmi_parse_cpbf(const char* str) {
    gsm_pb_entry_t* e;

    if (gsm.msg == NULL || gsm.msg->cmd_def != GSM_CMD_CPBF ||
        gsm.msg->msg.pb_search.ei >= gsm.msg->msg.pb_search.etr) {
        return 0;
    }

    if (*str == '+') {
        str += 7;
    }
    
    e = &gsm.msg->msg.pb_search.entries[gsm.msg->msg.pb_search.ei];
    e->pos = GSM_SZ(gsmi_parse_number(&str));
    gsmi_parse_string(&str, e->name, sizeof(e->name), 1);
    e->type = (gsm_number_type_t)gsmi_parse_number(&str);
    gsmi_parse_string(&str, e->number, sizeof(e->number), 1);

    gsm.msg->msg.pb_search.ei++;
    if (gsm.msg->msg.pb_search.er != NULL) {
        *gsm.msg->msg.pb_search.er = gsm.msg->msg.pb_search.ei;
    }
    return 1;
}

#endif /* GSM_CFG_PHONEBOOK || __DOXYGEN__ */
