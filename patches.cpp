#include "patches.h"


#include "f4sE_common/Relocation.h"
#include "F4SE_common/SafeWrite.h"
#include "f4se_common/BranchTrampoline.h"

#include "xbyak/xbyak.h"


namespace patches {

	RelocAddr<std::uint64_t> invJumpFrom(0x2567664);
	RelocAddr<std::uint64_t> invJumpTo(0x256766a);
	RelocAddr<std::uint64_t> toJumpFrom(0x1b932ea);
	RelocAddr<std::uint64_t> toJumpTo(0x1b932f2);
	RelocAddr<std::uint64_t> toJumpBreak(0x1b93315);

	RelocAddr<std::uint64_t> lockForRead_branch(0x1b932f8);
	RelocAddr<std::uint64_t> lockForRead_return(0x1b932fd);

	RelocAddr<std::uint64_t> shaderEffectPatch(0x28d323a);
	RelocAddr<std::uint64_t> shaderEffectCall(0x2813560);
	RelocAddr<std::uint64_t> shaderEffectContinue(0x28d323f);
	RelocAddr<std::uint64_t> shaderEffectReturn(0x28d4ec8);

	void patchInventoryInfBug() {

		struct PatchShortVar : Xbyak::CodeGenerator {
			PatchShortVar(void* buf) : Xbyak::CodeGenerator(2048, buf) {
				Xbyak::Label retLab;
				
				and (edi, 0xffff);   // edi is an int but should be treated as a short.  Should allow for loop to exit.

				mov(r12d, 0xffff);
				jmp(ptr[rip + retLab]);

				L(retLab);
				dq(invJumpTo.GetUIntPtr());

			}
		};

		void* buf = g_localTrampoline.StartAlloc();
		PatchShortVar code(buf);
		g_localTrampoline.EndAlloc(code.getCurr());

		g_branchTrampoline.Write6Branch(invJumpFrom.GetUIntPtr(), uintptr_t(code.getCode()));


		//struct PatchTimeOut : Xbyak::CodeGenerator {
		//	PatchTimeOut(void* buf) : Xbyak::CodeGenerator(4096, buf) {
		//		Xbyak::Label retLab;
		//		Xbyak::Label retLab2;
		//		Xbyak::Label retLab3;
	
		//		cmp(ebx, 0x2710);
		//		jnc(retLab);
		//		jmp(ptr[rip + retLab3]);
		//		
		//		L(retLab);
		//		push(rax);
		//		mov(eax, ptr[rdi + 4]);
		////		and (eax, 0xBFFF);
		//		mov(ptr[rdi + 4], eax);
		//		pop(rax);
		//		jmp(ptr[rip + retLab2]);

		//		L(retLab2);
		//		dq(toJumpBreak.GetUIntPtr());

		//		L(retLab3);
		//		dq(toJumpTo.GetUIntPtr());

		//	}
		//};

		//void* buf2 = g_localTrampoline.StartAlloc();
		//PatchTimeOut code2(buf2);
		//g_localTrampoline.EndAlloc(code2.getCurr());

		//g_branchTrampoline.Write6Branch(toJumpFrom.GetUIntPtr(), uintptr_t(code2.getCode()));


		return;
	}

	void patchLockForReadMask() {
		struct PatchMoreMask : Xbyak::CodeGenerator {
			PatchMoreMask(void* buf) : Xbyak::CodeGenerator(2048, buf) {
				Xbyak::Label retLab;

				and (dword[rdi + 0x4], 0xFFFFFFF);
				mov(rcx, 1);
				jmp(ptr[rip + retLab]);
				
				L(retLab);
				dq(lockForRead_return.GetUIntPtr());


			}
		};

		void* buf = g_localTrampoline.StartAlloc();
		PatchMoreMask code(buf);
		g_localTrampoline.EndAlloc(code.getCurr());

		g_branchTrampoline.Write5Branch(lockForRead_branch.GetUIntPtr(), uintptr_t(code.getCode()));
	}

	void patchPipeGunScopeCrash() {
		struct PatchMissingR15 : Xbyak::CodeGenerator {
			PatchMissingR15(void* buf) : Xbyak::CodeGenerator(4096, buf) {
				Xbyak::Label retLab;
				Xbyak::Label contLab;


				mov(r15, ptr[rsi + 0x78]);
				test(r15,r15);
				jz("null_pointer");
				mov(rax, shaderEffectCall.GetUIntPtr());
				call(rax);
				jmp(ptr[rip + contLab]);

				L("null_pointer");
				jmp(ptr[rip + retLab]);

				L(retLab);
				dq(shaderEffectReturn.GetUIntPtr());

				L(contLab);
				dq(shaderEffectContinue.GetUIntPtr());
			}
		};

		void* buf = g_localTrampoline.StartAlloc();
		PatchMissingR15 code(buf);
		g_localTrampoline.EndAlloc(code.getCurr());

		g_branchTrampoline.Write5Branch(shaderEffectPatch.GetUIntPtr(), uintptr_t(code.getCode()));
	}


	bool patchAll() {
		patchInventoryInfBug();
		patchLockForReadMask();
		patchPipeGunScopeCrash();
		return true;
	}

}