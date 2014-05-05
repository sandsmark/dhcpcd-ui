/*
 * libdhcpcd
 * Copyright 2009-2014 Roy Marples <roy@marples.name>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libintl.h>

#define IN_LIBDHCPCD
#include "libdhcpcd.h"

#define _ gettext

char *
dhcpcd_if_message(const DHCPCD_IF *i)
{
	char *msg, *p;
	const char *reason = NULL;
	size_t len;
	bool showip, showssid;
	char buf[INET6_ADDRSTRLEN];

	/* Don't report non SLAAC configurations */
	if (strcmp(i->type, "ra") == 0 &&
	    IN6_IS_ADDR_UNSPECIFIED(&i->prefix) &&
	    i->up)
		return NULL;

	showip = true;
	showssid = false;
	if (strcmp(i->reason, "EXPIRE") == 0)
		reason = _("Expired");
	else if (strcmp(i->reason, "CARRIER") == 0) {
		if (i->wireless) {
			reason = _("Associated with");
			if (*i->ssid != '\0')
				showssid = true;
		} else
			reason = _("Cable plugged in");
		showip = false;
	} else if (strcmp(i->reason, "NOCARRIER") == 0) {
		if (i->wireless) {
			if (*i->ssid != '\0' || i->ip.s_addr != 0) {
				reason = _("Disassociated from");
				showssid = true;
			} else
				reason = _("Not associated");
		} else
			reason = _("Cable unplugged");
		showip = false;
	} else if (strcmp(i->reason, "FAIL") == 0)
		reason = _("Automatic configuration not possible");
	else if (strcmp(i->reason, "3RDPARTY") == 0)
		reason = _("Waiting for 3rd Party configuration");

	if (reason == NULL) {
		if (i->up)
			reason = _("Configured");
		else if (strcmp(i->type, "ra") == 0)
			reason = "Expired RA";
		else
			reason = i->reason;
	}

	len = strlen(i->ifname) + 3;
	len += strlen(reason) + 1;
	if (i->ip.s_addr != 0) {
		len += 16; /* 000. * 4 */
		if (i->cidr != 0)
			len += 3; /* /32 */
	} else if (!IN6_IS_ADDR_UNSPECIFIED(&i->ip6)) {
		len += INET6_ADDRSTRLEN;
	} else if (!IN6_IS_ADDR_UNSPECIFIED(&i->prefix)) {
		len += INET6_ADDRSTRLEN;
		if (i->prefix_len != 0)
			len += 4;
	}
	if (showssid)
		len += strlen(i->ssid) + 1;
	msg = p = malloc(len);
	if (msg == NULL)
		return NULL;
	p += snprintf(msg, len, "%s: %s", i->ifname, reason);
	if (showssid)
		p += snprintf(p, len - (size_t)(p - msg), " %s", i->ssid);
	if (i->ip.s_addr != 0 && showip) {
		p += snprintf(p, len - (size_t)(p - msg), " %s", inet_ntoa(i->ip));
		if (i->cidr != 0)
			snprintf(p, len - (size_t)(p - msg), "/%d", i->cidr);
	} else if (!IN6_IS_ADDR_UNSPECIFIED(&i->ip6) && showip) {
		p += snprintf(p, len - (size_t)(p - msg), " %s",
		    inet_ntop(AF_INET6, &i->ip6, buf, INET6_ADDRSTRLEN));
	} else if (!IN6_IS_ADDR_UNSPECIFIED(&i->prefix) && showip) {
		p += snprintf(p, len - (size_t)(p - msg), " %s",
		    inet_ntop(AF_INET6, &i->prefix, buf, INET6_ADDRSTRLEN));
		if (i->prefix_len != 0)
			snprintf(p, len - (size_t)(p - msg), "/%d", i->prefix_len);
	}
	return msg;
}
