#pragma once

void memDump(uint8_t *pAddr, size_t len) {
	g_pSmmAPI->ConPrintf("Start dump at: %p\n", pAddr);
	size_t llen = len;
	while(len--) {
		g_pSmmAPI->ConPrintf("%02x", *pAddr++ & 0xFF);
	}
	g_pSmmAPI->ConPrintf("\nDump end. Next byte: %p. Len: %d\n", pAddr, llen);
}