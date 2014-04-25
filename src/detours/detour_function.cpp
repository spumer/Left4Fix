#include "asm/asm.h"
#include "detour_function.h"
#include "CDetour/detourhelpers.h"


DetourFunction::~DetourFunction() {}


void DetourFunction::PatchFromAddress(void *targetFunction, unsigned char *&originalFunction, unsigned char *&signature)
{
	L4D_DEBUG_LOG("Detour -- beginning patch routine on address %p", signature);

	//copy the original func's first few bytes into the trampoline
	int copiedBytes = copy_bytes(/*src*/signature, /*dest*/NULL, OP_JMP_SIZE);

	assert(copiedBytes >= OP_JMP_SIZE);

	// create the jmp to our detour function
	patch_t detourJmpPatch;
	detourJmpPatch.bytes = copiedBytes;
	InjectJmp(detourJmpPatch.patch, signature, targetFunction);
	fill_nop(detourJmpPatch.patch + OP_JMP_SIZE, copiedBytes - OP_JMP_SIZE);

	trampoline = (unsigned char*) spengine->AllocatePageMemory(copiedBytes + OP_JMP_SIZE);
	//bugfix: when copying to trampoline call/jmp edges must be fixed up
	copy_bytes(/*src*/signature, /*dest*/trampoline, copiedBytes);
	L4D_DEBUG_LOG("Detour -- Copied %d bytes to trampoline @ %p", copiedBytes, trampoline);

	//at end of trampoline, place jmp back to resume spot of the original func
	inject_jmp(/*src*/trampoline + copiedBytes, /*dest*/signature + copiedBytes);
	//NOTE: above will 'break' if there is any JMP that goes into the first copiedBytes anywhere else :(
	ApplyPatch(signature, /*offset*/0, &detourJmpPatch, m_pRestore);
	originalFunction = trampoline;

	L4D_DEBUG_LOG("Detour has been patched for function @ %p", signature);
}


void DetourFunction::Unpatch()
{
	//NOTE: careful not to call any vfuncs here since its invoked by destructor

	L4D_DEBUG_LOG("DetourFunction::Unpatch()");
	if(!isPatched) return;

	Detour::Unpatch();
	
	L4D_DEBUG_LOG("DetourFunction::Unpatch() -- restoring %s to original state", signatureName);
	ApplyPatch(signature, /*offset*/0, m_pRestore, /*restore*/NULL);
	
	L4D_DEBUG_LOG("DetourFunction::Unpatch() -- freeing trampoline");
	spengine->FreePageMemory(trampoline);

	L4D_DEBUG_LOG("DetourFunction %s has been unpatched", signatureName);
	isPatched = false;
}


//insert a specific JMP instruction at the given location, save it to the buffer
void DetourFunction::InjectJmp(void *buffer, void* src, void* dest) {
	*(unsigned char*)buffer = OP_JMP;
	*(long*)((unsigned char*)buffer+1) = (long)((unsigned char*)dest - ((unsigned char*)src + OP_JMP_SIZE));
}
