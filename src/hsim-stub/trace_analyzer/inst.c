#include "inst.h"

void init_inst(Inst *inst, uint32_t seq, uint32_t pc, uint32_t code){
    inst->seq = seq;
    inst->pc = pc;
    inst->code = code;
    inst->state = Ready;
    inst->type = IntAlu;

	inst->is_cond_inst = false;
	inst->update_cond_flag = false;
	inst->shift_inst = false;
	inst->stack_inst = false;
	inst->mshr = false;

	inst->cache_miss = L1Hit;
	inst->bpred_mispred = false;
	
	inst->num_src_reg = 0;
	inst->num_dst_reg = 0;
	inst->num_eff_addr = 0;
	inst->iq_elem = NULL;
	inst->ldq_elem = NULL;
	inst->stq_elem = NULL;

	inst->decoded = false;

	inst->imm = 0;
	inst->str = NULL;



}
void init_inst_type(InstType *type, InstTypeName name, int latency, bool pipelined){
	type->name = name;
	type->latency = latency;
	type->pipelined = pipelined;
}


void init_inst_src_dst_reg(Inst *inst, 	darm_t *d){
	int i=0;
	int regs[16];
	int num_reglist = darm_reglist2(d->reglist, regs);


	for (i=0 ; i<16 ; i++){
		inst->src_reg[i] = -1;
		inst->dst_reg[i] = -1;
//		inst->raw_inst[i] = NULL;

	}

	if (inst->type == MemRead || inst->type == Pop){
		if (d->Rt  != R_INVLD){
			inst->dst_reg[inst->num_dst_reg++] =  d->Rt;
		}
		if (d->Rn  != R_INVLD){
			inst->src_reg[inst->num_src_reg] =  d->Rn;
//			inst->raw_inst[inst->num_src_reg] = NULL;
			inst->num_src_reg++;
		}
		for (i=0 ; i< num_reglist ; i++){
			inst->dst_reg[inst->num_dst_reg++] =  regs[i];
		}
	}
	else if (inst->type == MemWrite || inst->type == Push){
		if (d->Rt  != R_INVLD){
			inst->src_reg[inst->num_src_reg] =  d->Rt;
//			inst->raw_inst[inst->num_src_reg] = NULL;
			inst->num_src_reg++;
		}

		if (d->Rn  != R_INVLD){
			inst->src_reg[inst->num_src_reg] =  d->Rn;
//			inst->raw_inst[inst->num_src_reg] = NULL;
			inst->num_src_reg++;
		}
		for (i=0 ; i< num_reglist ; i++){
			inst->src_reg[inst->num_src_reg] =  regs[i];
//			inst->raw_inst[inst->num_src_reg] = NULL;
			inst->num_src_reg++;
		}
/*
		if (inst->stack_inst){
			inst->dst_reg[inst->num_dst_reg++] =  13;
			inst->src_reg[inst->num_src_reg++] =  13;
		}
*/
	}
	else {
		if (d->Rd  != R_INVLD){
			inst->dst_reg[inst->num_dst_reg++] =  d->Rd;

		}
		if (d->Rm  != R_INVLD){
			inst->src_reg[inst->num_src_reg] =  d->Rm;
//			inst->raw_inst[inst->num_src_reg] = NULL;
			inst->num_src_reg++;

		}
		if (d->Rn  != R_INVLD){
			inst->src_reg[inst->num_src_reg] =  d->Rn;
//			inst->raw_inst[inst->num_src_reg] = NULL;
			inst->num_src_reg++;

		}
		for (i=0 ; i< num_reglist ; i++){
			inst->src_reg[inst->num_src_reg] =  regs[i];
//			inst->raw_inst[inst->num_src_reg] = NULL;
			inst->num_src_reg++;
		}
	}

	if (inst->is_cond_inst){
		inst->src_reg[inst->num_src_reg] =  17;
//		inst->raw_inst[inst->num_src_reg] = NULL;
		inst->num_src_reg++;
	}

	if (inst->update_cond_flag){
		inst->dst_reg[inst->num_dst_reg] =  17;
		inst->num_dst_reg++;
	}


/*
	if (inst->type == Branch){
		inst->dst_reg[inst->num_dst_reg] =  15;
		inst->num_dst_reg++;
	}
	*/
}

void set_inst_bpred_mispred(Inst *inst, bool mispred){
	inst->bpred_mispred = mispred;

}

void set_inst_eff_addr(Inst *inst, uint32_t addr, CacheMissType cache_miss){
	inst->eff_addr[inst->num_eff_addr] = addr;
	inst->num_eff_addr++;
	inst->cache_miss = cache_miss > inst->cache_miss ? cache_miss : inst->cache_miss;		//choose bigger one.
}
void decode_inst(Inst *inst){

	if (inst->decoded)
		return;

	darm_t d;
	darm_armv7_disasm (&d, inst->code);

	if (d.cond == C_UNCOND || d.cond == C_INVLD || d.cond == C_AL){
		inst->is_cond_inst = false;
	}
	else {
		inst->is_cond_inst = true;
	}
	if (d.shift)
		//|| d.Rs != R_INVLD)
		inst->shift_inst = true;

	if (d.S != B_INVLD){
		if (d.S == B_SET){
			inst->update_cond_flag = true;
		}
		else {
			inst->update_cond_flag = false;

		}
	}

	//char buf1[255];
	//printf("inst) %s %d %d \n", print_inst(buf1, inst), inst->is_cond_inst, inst->update_cond_flag);


	switch(d.instr){
		case I_AND:
		case I_EOR:
		case I_SUB:
		case I_RSB:
		case I_ADD:
		case I_ADC:
		case I_SBC:
		case I_RSC:
		case I_ORR:
		case I_MOVW:
		case I_BIC:
		case I_MVN:
		case I_MOVT:
		case I_MOV:
		case I_SSUB8:
		case I_QSUB8:
		case I_SHSUB8:
		case I_USUB8:
		case I_UQSUB8:
		case I_UHSUB8:
		case I_SEL:
		case I_REV16:
		case I_REVSH:
		case I_SBFX:
		case I_BFI:
		case I_UBFX:
		case I_CLZ:
			inst->type = IntAlu;
			break;

		//shift
		
		case I_ASR:
		case I_LSL:
		case I_LSR:
		case I_ROR:
		case I_RRX:
			inst->type = IntAlu;
			inst->shift_inst = true;

			break;

		case I_TST:
		case I_TEQ:
		case I_CMP:
		case I_CMN:
			inst->type = Branch;
			inst->update_cond_flag = true;
			break;

		case I_MUL:
		case I_MLA:
		case I_UMAAL:
		case I_MLS:
		case I_SMLA:
		case I_SMLABB:
		case I_SMLABT:
		case I_SMLATB:
		case I_SMLATT:
		case I_UMULL:
		case I_UMLAL:
		case I_SMUAD:
		case I_SMUL:
		case I_SMULBB:
		case I_SMULBT:
		case I_SMULL:
		case I_SMULTB:
		case I_SMULTT:
		case I_SMULW:
			inst->type = IntMult;
			break;

		case I_SDIV:
		case I_UDIV:
			inst->type = IntDiv;
			break;


		case I_VADD:
		case I_VADDHN:
		case I_VADDL:
		case I_VADDW:
		case I_VSUB:
		case I_VSUBHN:
		case I_VSUBL:
		case I_VSUBW:
			inst->type = SimdFloatAdd;
			break;

		case I_VMUL:
		case I_VNMUL:
			inst->type = SimdFloatMult;
			break;
		
		case I_VMLA:
		case I_VMLAL:
		case I_VMLS:
		case I_VMLSL: 
		case I_VNMLA:
		case I_VNMLS:
			inst->type = SimdFloatMultAcc;
			break;

		case I_LDM:
		case I_LDMDA:
		case I_LDMDB:
		case I_LDMIB:
		case I_LDR:
		case I_LDRB:
		case I_LDRBT:
		case I_LDRD:
//		case I_LDREX:
//		case I_LDREXB:
//		case I_LDREXD:
//		case I_LDREXH:
		case I_LDRH:
		case I_LDRHT:
		case I_LDRSB:
		case I_LDRSBT:
		case I_LDRSH:
		case I_LDRSHT:
		case I_LDRT:
		case I_PLD:
			inst->type = MemRead;
		break;

		case I_STM:
		case I_STMDA:
		case I_STMDB:
		case I_STMIB:
		case I_STR:
		case I_STRB:
		case I_STRBT:
		case I_STRD:
//		case I_STREX:
//		case I_STREXB:
//		case I_STREXD:
//		case I_STREXH:
		case I_STRH:
		case I_STRHT:
		case I_STRT:
			inst->type = MemWrite;
			break;
			
		case I_POP:
			inst->type = Pop;
			inst->stack_inst = true;
			break;

		case I_PUSH:
			inst->type = Push;
			inst->stack_inst = true;
			break;
		case I_B:
		case I_BL:
		case I_BX:
		case I_BXJ:
		case I_BLX:
			inst->type = Branch;
			break;
		case I_SVC:
		case I_MCR:
		case I_MRC:
			inst->type = Syscall;
			break;
		default:{
			inst->type = Unsupported;
			char buf[255];
			if (d.instr)
				fprintf(stderr, "Unsupported Instruction Type %X(%d) %s\n", inst->code, d.instr, print_inst(buf, inst));
		}
	}
	if (inst->type != Unsupported)
		init_inst_src_dst_reg(inst, &d);
	inst->decoded = true;
	inst->imm = d.imm;
}

char *print_inst(char *buf, Inst *inst){
	darm_t d;
	darm_str_t str;
	darm_armv7_disasm (&d, inst->code);
	darm_str2 (&d, &str, 1);

	if (inst->str){
	    sprintf(buf, "[%" PRIu64 "] %s ", inst->seq, inst->str);
	}

	else {
	    sprintf(buf, "[%" PRIu64 "] %s ", inst->seq, str.total);
	}
/*
	strcat(buf, "/src ");
	
	int i=0;
	for (i=0 ; i<inst->num_src_reg ; i++){
		char buf2[3];
		strcat(buf, " ");
		sprintf(buf2, "%d", inst->src_reg[i]);
		strcat(buf, buf2); 
		strcat(buf, " ");

	}
	strcat(buf, "/dst ");

	for (i=0 ; i<inst->num_dst_reg ; i++){
		char buf2[3];
		strcat(buf, " ");
		sprintf(buf2, "%d", inst->dst_reg[i]);
		strcat(buf, buf2); 
		strcat(buf, " ");
	}
	strcat(buf, "/addr ");

	for (i=0 ; i<inst->num_eff_addr ; i++){
		char buf2[3];
		strcat(buf, " ");
		sprintf(buf2, "%X", inst->eff_addr[i]);
		strcat(buf, buf2); 
		strcat(buf, " ");
	}
	
*/
    return buf;


}



