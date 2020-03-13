#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include "computer.h"
#undef mips			/* gcc already has a def for mips */

enum Instr {
    noop    = 0x00, // opcode

    // R-type
    addu    = 0x21, // funct
    subu    = 0x23, // funct
    sll     = 0x00, // funct
    srl     = 0x02, // funct
    and     = 0x24, // funct
    or      = 0x25, // funct
    slt     = 0x2a, // funct
    jr      = 0x08, // funct

    // I-type
    addiu   = 0x09, // opcode
    andi    = 0x0c, // opcode
    ori     = 0x0d, // opcode
    lui     = 0x0f, // opcode
    beq     = 0x04, // opcode
    bne     = 0x05, // opcode
    lw      = 0x23, // opcode
    sw      = 0x2b, // opcode

    // J-type
    j       = 0x02, // opcode
    jal     = 0x03  // opcode
};


unsigned int endianSwap(unsigned int);

void PrintInfo (int changedReg, int changedMem);
unsigned int Fetch (int);
void Decode (unsigned int, DecodedInstr*, RegVals*);
int Execute (DecodedInstr*, RegVals*);
int Mem(DecodedInstr*, int, int *);
void RegWrite(DecodedInstr*, int, int *);
void UpdatePC(DecodedInstr*, int);
void PrintInstruction (DecodedInstr*);

/*Globally accessible Computer variable*/
Computer mips;
RegVals rVals;

/*
 *  Return an initialized computer with the stack pointer set to the
 *  address of the end of data memory, the remaining registers initialized
 *  to zero, and the instructions read from the given file.
 *  The other arguments govern how the program interacts with the user.
 */
void InitComputer (FILE* filein, int printingRegisters, int printingMemory,
  int debugging, int interactive) {
    int k;
    unsigned int instr;

    /* Initialize registers and memory */

    for (k=0; k<32; k++) {
        mips.registers[k] = 0;
    }
    
    /* stack pointer - Initialize to highest address of data segment */
    mips.registers[29] = 0x00400000 + (MAXNUMINSTRS+MAXNUMDATA)*4;

    for (k=0; k<MAXNUMINSTRS+MAXNUMDATA; k++) {
        mips.memory[k] = 0;
    }

    k = 0;
    while (fread(&instr, 4, 1, filein)) {
	/*swap to big endian, convert to host byte order. Ignore this.*/
        mips.memory[k] = ntohl(endianSwap(instr));
        k++;
        if (k>MAXNUMINSTRS) {
            fprintf (stderr, "Program too big.\n");
            exit (1);
        }
    }

    mips.printingRegisters = printingRegisters;
    mips.printingMemory = printingMemory;
    mips.interactive = interactive;
    mips.debugging = debugging;
}

unsigned int endianSwap(unsigned int i) {
    return (i>>24)|(i>>8&0x0000ff00)|(i<<8&0x00ff0000)|(i<<24);
}

/*
 *  Run the simulation.
 */
void Simulate () {
    char s[40];  /* used for handling interactive input */
    unsigned int instr;
    int changedReg=-1, changedMem=-1, val;
    DecodedInstr d;
    
    /* Initialize the PC to the start of the code section */
    mips.pc = 0x00400000;
    while (1) {
        if (mips.interactive) {
            printf ("> ");
            fgets (s,sizeof(s),stdin);
            if (s[0] == 'q') {
                return;
            }
        }

        /* Fetch instr at mips.pc, returning it in instr */
        instr = Fetch (mips.pc);

        printf ("Executing instruction at %8.8x: %8.8x\n", mips.pc, instr);

        /* 
	 * Decode instr, putting decoded instr in d
	 * Note that we reuse the d struct for each instruction.
	 */
        Decode (instr, &d, &rVals);

        /*Print decoded instruction*/
        PrintInstruction(&d);

        /* 
	 * Perform computation needed to execute d, returning computed value 
	 * in val 
	 */
        val = Execute(&d, &rVals);

	UpdatePC(&d,val);

        /* 
	 * Perform memory load or store. Place the
	 * address of any updated memory in *changedMem, 
	 * otherwise put -1 in *changedMem. 
	 * Return any memory value that is read, otherwise return -1.
         */
        val = Mem(&d, val, &changedMem);

        /* 
	 * Write back to register. If the instruction modified a register--
	 * (including jal, which modifies $ra) --
         * put the index of the modified register in *changedReg,
         * otherwise put -1 in *changedReg.
         */
        RegWrite(&d, val, &changedReg);

        PrintInfo (changedReg, changedMem);
    }
}

/*
 *  Print relevant information about the state of the computer.
 *  changedReg is the index of the register changed by the instruction
 *  being simulated, otherwise -1.
 *  changedMem is the address of the memory location changed by the
 *  simulated instruction, otherwise -1.
 *  Previously initialized flags indicate whether to print all the
 *  registers or just the one that changed, and whether to print
 *  all the nonzero memory or just the memory location that changed.
 */
void PrintInfo ( int changedReg, int changedMem) {
    int k, addr;
	//printf("%d\n", mips.pc);
    printf ("New pc = %8.8x\n", mips.pc);
    if (!mips.printingRegisters && changedReg == -1) {
        printf ("No register was updated.\n");
    } else if (!mips.printingRegisters) {
        printf ("Updated r%2.2d to %8.8x\n",
        changedReg, mips.registers[changedReg]);
    } else {
        for (k=0; k<32; k++) {
            printf ("r%2.2d: %8.8x  ", k, mips.registers[k]);
            if ((k+1)%4 == 0) {
                printf ("\n");
            }
        }
    }
    if (!mips.printingMemory && changedMem == -1) {
        printf ("No memory location was updated.\n");
    } else if (!mips.printingMemory) {
        printf ("Updated memory at address %8.8x to %8.8x\n",
        changedMem, Fetch (changedMem));
    } else {
        printf ("Nonzero memory\n");
        printf ("ADDR	  CONTENTS\n");
        for (addr = 0x00400000+4*MAXNUMINSTRS;
             addr < 0x00400000+4*(MAXNUMINSTRS+MAXNUMDATA);
             addr = addr+4) {
            if (Fetch (addr) != 0) {
                printf ("%8.8x  %8.8x\n", addr, Fetch (addr));
            }
        }
    }
}



/*
 *  Return the contents of memory at the given address. Simulates
 *  instruction fetch. 
 */
unsigned int Fetch ( int addr) {
    return mips.memory[(addr-0x00400000)/4];
}

/* Decode instr, returning decoded instruction. */
void Decode ( unsigned int instr, DecodedInstr* d, RegVals* rVals) {
    /* Your code goes here */
	int opcode = 0;
	int rs = 0;
	int rt = 0;
	int rd = 0;
	int shamt = 0;
	unsigned int function = 0;
	int immediates = 0;

	// geting the opcode, first 6 bit
	opcode = instr>>26;
	d->op = opcode;
	// after the first shift there are zeros...

	//J-format
	if(opcode == 2 || opcode == 3){
		int temp = 0;
		//d->op = opcode;
		d->type = J;
		//getting the location
		// eliminate the opcode 26adddress + 6zeros,
		temp = instr<<6>>4;
		//printf("1.%x\n",temp);
		//temp = instr & (0xF0000000);
		//printf("2.%x\n",temp);
		//which I only need 2 zeros
		//temp = temp>>2;
		//printf("3.%x\n ",temp);
		d->regs.j.target = temp;
		//printf("%x\n3 ",temp);

	}

	//R-format
	else if(opcode == 0){
		//d->op = opcode;
		//type
		d->type	= R;
		//rs, eliminate opcde and all element after rs; 6
		rs = instr<<6;
		d->regs.r.rs = rs>>27;
		//rt eliminte opcode rs and all other element after rt; 6+5
		rt = instr<<11;
		d->regs.r.rt = rt>>27;
		//rd same as above 6+5+5
		rd = instr<<16;
		d->regs.r.rd = rd>>27;
		//shamt same as above 6+5+5+5
		shamt = instr<<21;
		d->regs.r.shamt = shamt>>27;
		// function same as above 6+5+5+5+5 6bit required
		function = instr<<26;
		d->regs.r.funct = function>>26;
       		rVals->R_rs = mips.registers[d->regs.r.rs];
        	rVals->R_rt = mips.registers[d->regs.r.rt];
        	rVals->R_rd = mips.registers[d->regs.r.rd];

	}
	//I-format
	else{
		//d->op = opcode;
		//type
		d->type	= I;
		//rs, eliminate opcde and all element after rs; 6
		rs = instr<<6;
		d->regs.i.rs = rs>>27;
		//rt eliminte opcode rs and all other element after rt; 6+5
		rt = instr<<11;
		d->regs.i.rt = rt>>27;
		//immediates
		//int temp = 0;
			//temp=instr<<2;
		//temp;

		immediates = instr & (0x0000ffff);
		//positive  16->32bit address
			if(immediates>>15 == 0){ 

			//bitwise and
				//d->regs.i.addr_or_immed
				d->regs.i.addr_or_immed = immediates & (0x0000ffff);
			}
		//negative 
			else {
			//printf("hello\n");
			//bitwise or
				d->regs.i.addr_or_immed = immediates | (0xffff0000);
			}

		rVals->R_rs = mips.registers[d->regs.i.rs];
        	rVals->R_rt = mips.registers[d->regs.i.rt];
        	//rVals->R_rd = mips.registers[d->regs.r.rd];

	//}
		/*
		else{
			immediates = immediates <<2;
			immediates = immediates >> 6;
			printf("%x\n ",immediates);			
		}
		*/
			//}


			//immediates = instr & (0x0000ffff);
	}

}

/*
 *  Print the disassembled version of the given instruction
 *  followed by a newline.
 */
void PrintInstruction ( DecodedInstr* d) {
    /* Your code goes here */

	//j-format
	if(d->op == 2 || d->op == 3){
			//j
			if(d->op == 2){
		   		printf("j\t");
				printf("0x%.8x\n",d->regs.j.target);
			}
			//jal
			else {
		   		printf("jal\t");
				printf("0x%.8x\n", d->regs.j.target);
			}
	}

	//r-format
	else if(d->op == 0){
		//sll
		if(d->regs.r.funct == 0){
                  	printf("sll\t");
			printf("$%d, ", d->regs.r.rd);
			printf("$%d, ", d->regs.r.rt);
			printf("$%d\n",d->regs.r.shamt);
		}
		//srl
		else if(d->regs.r.funct == 2){
                  	printf("srl\t");
			printf("$%d, ", d->regs.r.rd);
			printf("$%d, ", d->regs.r.rt);
			printf("$%d\n",d->regs.r.shamt);
		}
		//jr
		else if(d->regs.r.funct == 8){
                  	printf("jr\t");
			printf("$%d", d->regs.r.rs);
		}
		//addu
		else if(d->regs.r.funct == 33){
			printf("addu\t");
			printf("$%d, ", d->regs.r.rd);
			printf("$%d, ", d->regs.r.rs);
			printf("$%d\n",d->regs.r.rt);
		}
		//subu
		else if(d->regs.r.funct == 35){
                  	printf("subu\t");
			printf("$%d, ", d->regs.r.rd);
			printf("$%d, ", d->regs.r.rs);
			printf("$%d\n",d->regs.r.rt);
		}
		//and
		else if(d->regs.r.funct == 36){
                  	printf("and\t");
			printf("$%d, ", d->regs.r.rd);
			printf("$%d, ", d->regs.r.rs);
			printf("$%d\n",d->regs.r.rt);
		}
		//or
		else if(d->regs.r.funct == 37){
                  	printf("or\t");
			printf("$%d, ", d->regs.r.rd);
			printf("$%d, ", d->regs.r.rs);
			printf("$%d\n",d->regs.r.rt);
		}
		//slt
		else if(d->regs.r.funct == 42){
                  	printf("slt\t");
			printf("$%d, ", d->regs.r.rd);
			printf("$%d, ", d->regs.r.rs);
			printf("$%d\n",d->regs.r.rt);
		}
		//instruction doesn't exist
		else{
			printf("Instruction doesn't exist");
			exit(0);
		}

	}

	//i-format
	else{
			int temp = 0;
		//beq
		if(d->op == 4){
                  	printf("beq\t");
			printf("$%d, ", d->regs.r.rs);
			printf("$%d, ", d->regs.r.rt);
			temp = d->regs.i.addr_or_immed;
			temp = mips.pc + 4 + ( temp << 2);
			printf("0x%8.8x\n",temp);
		}
		//bne
		else if(d->op == 5){
                  	printf("bne\t");
			printf("$%d, ", d->regs.r.rs);
			printf("$%d, ", d->regs.r.rt);
			temp = d->regs.i.addr_or_immed;
			temp = mips.pc + 4 + ( temp << 2);
			printf("0x%8.8x\n",temp);
		}
		//addiu
		else if(d->op == 9){
                  	printf("addiu\t");
			printf("$%d, ", d->regs.r.rt);
			printf("$%d, ", d->regs.r.rs);
			printf("%d\n",d->regs.i.addr_or_immed);
		}
		//andi
		else if(d->op == 12){
                  	printf("andi\t");
			printf("$%d, ", d->regs.r.rt);
			printf("$%d, ", d->regs.r.rs);
			printf("$%x\n",d->regs.i.addr_or_immed);
		}
		//ori
		else if(d->op == 13){
                  	printf("ori\t");
			printf("$%d, ", d->regs.r.rt);
			printf("$%d, ", d->regs.r.rs);
			printf("$%x\n",d->regs.i.addr_or_immed);
		}
		//lui
		else if(d->op == 15){
                  	printf("lui\t");
			printf("$%d, ", d->regs.r.rt);
			printf("$%x\n",d->regs.i.addr_or_immed);
		}
		//lw
		else if(d->op == 35){
                  	printf("lw\t");
			printf("$%d, ", d->regs.r.rt);
			printf("%x",d->regs.i.addr_or_immed);
			printf("($%d)\n), ", d->regs.r.rs);
		}
		//sw
		else if(d->op == 43){
                  	printf("sw\t");
			printf("$%d, ", d->regs.r.rt);
			printf("%x",d->regs.i.addr_or_immed);
			printf("($%d)\n), ", d->regs.r.rs);

			
		}
		//instruction doesn't exist
		else{
			printf("Instruction doesn't exist");
			exit(0);
		}

	}


}

/* Perform computation needed to execute d, returning computed value */
int Execute ( DecodedInstr* d, RegVals* rVals) {
    /* Your code goes here */
	//j-format
	if(d->op == 2 || d->op == 3){
		//j
		if(d->op == 2){
		   	return (mips.pc + 4);
		}
		//jal
		else {
		   	return (mips.pc + 4);
		}
	}

	//r-format
	else if(d->op == 0){
		//sll
		if(d->regs.r.funct == 0){
                  	return (mips.registers[d->regs.r.rt] << d->regs.r.shamt);
		}
		//srl
		else if(d->regs.r.funct == 2){
                  	return (mips.registers[d->regs.r.rt] >> d->regs.r.shamt);
		}
		//jr
		else if(d->regs.r.funct == 8){
                  	return (mips.registers[31]);
		}
		//addu
		else if(d->regs.r.funct == 33){
			
			return (rVals->R_rs + rVals->R_rt);
		}
		//subu
		else if(d->regs.r.funct == 35){
                  	return (mips.registers[d->regs.r.rs] - mips.registers[d->regs.r.rt]);
		}
		//and
		else if(d->regs.r.funct == 36){
			//bitwise and
                  	return (mips.registers[d->regs.r.rs] & mips.registers[d->regs.r.rt]);
		}
		//or
		else if(d->regs.r.funct == 37){
			//bitwise or
                  	return (mips.registers[d->regs.r.rs] | mips.registers[d->regs.r.rt]);
		}
		//slt
		else if(d->regs.r.funct == 42){
                  	return (mips.registers[d->regs.r.rs] - mips.registers[d->regs.r.rt] < 0);
		}
		//instruction doesn't exist
		else{
			printf("Instruction doesn't exist");
			exit(0);
		}

	}

	//i-format
	else{
		//beq and bne
		if(d->op == 4 ){
           	 	if(mips.registers[d->regs.i.rs] - mips.registers[d->regs.i.rt] == 0){
				return 1;
			}
			else{
				return 0;
			}
			 
		}
			else if(d->op == 5){
				//bne
			if(mips.registers[d->regs.i.rs] - mips.registers[d->regs.i.rt] != 0){
				return 1;
			}
			else {
				return 0;
			}
		}
		


		//addiu
		else if(d->op == 9){
			//printf("hello world\n");
                  	return (mips.registers[rVals->R_rs] + d->regs.i.addr_or_immed);
		}
		//andi
		else if(d->op == 12){
                  	return (mips.registers[d->regs.i.rs] & d->regs.i.addr_or_immed);
		}
		//ori
		else if(d->op == 13){
                  	 return (mips.registers[d->regs.i.rs] | d->regs.i.addr_or_immed);
		}
		//lui
		else if(d->op == 15){
                  	return ((d->regs.i.addr_or_immed << 16) & 0xFFFF0000);
		}
		//lw
		else if(d->op == 35){
                  	return (mips.registers[d->regs.i.rs] - (d->regs.i.addr_or_immed/4+1)*4);
		}
		//sw
		else if(d->op == 43){
                  	return (mips.registers[d->regs.i.rs] - (d->regs.i.addr_or_immed/4+1)*4);
			
		}
		//instruction doesn't exist
		else{
			printf("Instruction doesn't exist");
			exit(0);
		}

	}


  return 0;
}

/* 
 * Update the program counter based on the current instruction. For
 * instructions other than branches and jumps, for example, the PC
 * increments by 4 (which we have provided).
 */
void UpdatePC(DecodedInstr *d, int val)
{
    mips.pc += 4;
    /* Your code goes here */
    unsigned int type;
    unsigned int opcode, funct, target;

    type = d->type;
    opcode = d->op;

    if (type == R)
    {
        funct = d->regs.r.funct;

        if (funct == jr)            mips.pc = val;
    }
    else if (type == I)
    {
        if (opcode == beq)          mips.pc += val;
        else if (opcode == bne)     mips.pc += val;
    }
    else if (type == J)
    {
        target = d->regs.j.target;
        
        mips.pc = target;
    }
}

/*
 * Perform memory load or store. Place the address of any updated memory 
 * in *changedMem, otherwise put -1 in *changedMem. Return any memory value 
 * that is read, otherwise return -1. 
 *
 * Remember that we're mapping MIPS addresses to indices in the mips.memory 
 * array. mips.memory[0] corresponds with address 0x00400000, mips.memory[1] 
 * with address 0x00400004, and so forth.
 *
 */
int Mem( DecodedInstr* d, int val, int *changedMem) {
    /* Your code goes here */
    
         //it is lw or sw so we have to do the load or store memory.
    if(d -> op == 35 || d -> op == 45){

        //only dealing with I format do we don't have to worry about checking for
        //for any other format

        //Initialize all the characteristics of the I-format
        int immediate = (d -> regs.i.addr_or_immed) * 4; //converting to bits
        int rs = mips.registers[d -> regs.i.rs]; //grabing the value of rs through the mips register
        //int rt = mips.registers[d -> regs.i.rt]; //grabing the value of rt through the mips register

        //if load word memory manage here
        if(d -> op == 35){

            //load word only changes the value of rt doesn't change the memory
            val = mips.memory[1024 + rs + immediate]; //mips cheat sheet [R[rs] + imm]
            *changedMem = -1;
            return val;


        }
        //if not then it can only be store word and memory manage here
        else{

            //store word changes the memory and sets rt
            
            //dividing by 4 to get the bits
           // rt = mips.memory[1024 + ((rs + immediate) / 4)];//mips cheat sheet [R[rs] + imm]
            int newrtvalue = 0x00401000 + rs + immediate; //didnt change val so we have to manually get the new value
            *changedMem = newrtvalue; //changing the memory to the new rt value
            
            return newrtvalue;

        }

    }
    else{ //then anything other than store word and load word we do no memory management

        //since no memory was updated we change it to -1
        *changedMem = -1;
        return val;

    }


    
  return 0;
}

/* 
 * Write back to register. If the instruction modified a register--
 * (including jal, which modifies $ra) --
 * put the index of the modified register in *changedReg,
 * otherwise put -1 in *changedReg.
 */
void RegWrite( DecodedInstr* d, int val, int *changedReg)
{
    /* Your code goes here */
    *changedReg = -1;

    unsigned int type;
    unsigned int opcode, rt, rd, funct;

    type = d->type;
    opcode = d->op;

    if (type == R)
    {
        rd = d->regs.r.rd;
        funct = d->regs.r.funct;

        if(funct != jr)
        {
            mips.registers[rd] = val;
            *changedReg = rd;
        }
    }
    else if (type == I)
    {
        rt = d->regs.i.rt;

        if(opcode != beq && opcode != bne && opcode != sw)
        {
            mips.registers[rt] = val;
            *changedReg = rt;
        }
    }
    else
    {
        if(opcode == jal)
        {
            mips.registers[31] = val;
            *changedReg = 31;
        }
    }
}
