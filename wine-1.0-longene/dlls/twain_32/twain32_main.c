/*
 * TWAIN32 functions
 *
 * Copyright 2000 Shi Quan He <shiquan@cyberdude.com>
 * Copyright 2006 Marcus Meissner
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "twain.h"
#include "twain_i.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(twain);

/* A helper function that looks up a destination identity in the active
   source list */
static activeDS *TWAIN_LookupSource (const TW_IDENTITY *pDest)
{
    activeDS *pSource;

    for (pSource = activeSources; pSource; pSource = pSource->next)
        if (pSource->identity.Id == pDest->Id)
            break;
    return pSource;
}

BOOL WINAPI DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("%p,%x,%p\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            DSM_currentState = 2;
            break;

        case DLL_PROCESS_DETACH:
            DSM_currentState = 1;
            break;
    }

    return TRUE;
}

TW_UINT16 TWAIN_SourceManagerHandler (
           pTW_IDENTITY pOrigin,
           TW_UINT16   DAT,
           TW_UINT16   MSG,
           TW_MEMREF   pData)
{
    TW_UINT16 twRC = TWRC_SUCCESS;

    switch (DAT)
    {
        case DAT_IDENTITY:
            switch (MSG)
            {
                case MSG_CLOSEDS:
                    twRC = TWAIN_CloseDS (pOrigin, pData);
                    break;

                case MSG_GETDEFAULT:
                    twRC = TWAIN_IdentityGetDefault (pOrigin, pData);
                    break;

                case MSG_GETFIRST:
                    twRC = TWAIN_IdentityGetFirst (pOrigin, pData);
                    break;

                case MSG_GETNEXT:
                    twRC = TWAIN_IdentityGetNext (pOrigin, pData);
                    break;

                case MSG_OPENDS:
                    twRC = TWAIN_OpenDS (pOrigin, pData);
                    break;

                case MSG_USERSELECT:
                    twRC = TWAIN_UserSelect (pOrigin, pData);
                    break;

                default:
                    /* Unrecognized operation triplet */
                    twRC = TWRC_FAILURE;
                    DSM_twCC = TWCC_BADPROTOCOL;
                    WARN("unrecognized operation triplet\n");
                    break;
            }
            break;

        case DAT_PARENT:
            switch (MSG)
            {
                case MSG_CLOSEDSM:
                    twRC = TWAIN_CloseDSM (pOrigin, pData);
                    break;

                case MSG_OPENDSM:
                    twRC = TWAIN_OpenDSM (pOrigin, pData);
                    break;

                default:
                    /* Unrecognized operation triplet */
                    twRC = TWRC_FAILURE;
                    DSM_twCC = TWCC_BADPROTOCOL;
                    WARN("unrecognized operation triplet\n");
            }
            break;

        case DAT_STATUS:
            if (MSG == MSG_GET) {
                twRC = TWAIN_GetDSMStatus (pOrigin, pData);
            } else {
                twRC = TWRC_FAILURE;
                DSM_twCC = TWCC_BADPROTOCOL;
                WARN("unrecognized operation triplet\n");
            }
            break;

        default:
            twRC = TWRC_FAILURE;
            DSM_twCC = TWCC_BADPROTOCOL;
            WARN("unrecognized operation triplet\n");
            break;
    }

    return twRC;
}


/* Main entry point for the TWAIN library */
TW_UINT16 WINAPI
DSM_Entry (pTW_IDENTITY pOrigin,
           pTW_IDENTITY pDest,
           TW_UINT32    DG,
           TW_UINT16    DAT,
           TW_UINT16    MSG,
           TW_MEMREF    pData)
{
    TW_UINT16 twRC = TWRC_SUCCESS;  /* Return Code */

    TRACE("(DG=%ld DAT=%d MSG=%d)\n", DG, DAT, MSG);

    if (pDest)
    {
        activeDS *pSource = TWAIN_LookupSource (pDest);
	/* This operation's destination is a source */

        if (!pSource) {
	    ERR("No source associated with pDest %p\n", pDest);
	    DSM_twCC = TWCC_BADDEST;
	    return TWRC_FAILURE;
	}
	DSM_twCC = TWCC_SUCCESS;
	TRACE("Forwarding %ld/%d/%d/%p to DS.\n", DG, DAT, MSG, pData);
	twRC = pSource->dsEntry(pOrigin, DG, DAT, MSG, pData);
	TRACE("return value is %d\n", twRC);
	return twRC;
    }
    switch (DG)
    {
        case DG_CONTROL:
            twRC = TWAIN_SourceManagerHandler (pOrigin, DAT, MSG, pData);
            break;
        default:
	    FIXME("The DSM does not handle DG %ld\n", DG);
            DSM_twCC = TWCC_BADPROTOCOL;
            twRC = TWRC_FAILURE;
    }
    return twRC;
}
