#include <iostream>
#include <vector>
#include <time.h>



//#define MYDEBUG(x) x
#define MYDEBUG(x) 
using namespace std;
//operation type
enum OP { F_SET, F_SUB, F_ADD, F_JMPNF, F_RET, F_LT, F_CALL, F_PRI};
//Argument type
enum VAR_TYPE { T_CONST=1, T_TEMP, T_ARG_IN, T_ARG_OUT, T_GLOB_VAR, T_STATIC, T_FUNC};

struct Prog;

//typedef unsigned char reg_;
struct Instr
{
	size_t op;	// op code
	size_t args[3];
	//reg_ args[3]; // three register by operation
};

struct Scope
{
	double *regs;
};
struct Prog
{
	Instr instrs[20];
	int Static_vars[2];
	double const_val[2];
	size_t narg_in;
	size_t narg_out;
	size_t n_temps;
	size_t n_const;
	int prg_len;
	Scope scope;
};

Prog prg_main;
Prog fib;
// function table
Prog* func_tab[5];


#define USE_POOL 1
#ifdef USE_POOL

std::vector<double > TempPool;
size_t pool_p = 0;

void AllocTemps(Prog* prg)
{
	prg->scope = {0};

	if (prg->n_temps >0)
	{
		prg->scope.regs = &TempPool[pool_p];
		pool_p += prg->n_temps;
	}
}
void FreeTemps(Prog* prg)
{
	if (prg->n_temps >0)
	{
		pool_p -= prg->n_temps;
	}
}
#else
void AllocTemps(Prog* prg)
{
	prg->scope = {};
	if (prg->n_regs > 0)
		prg->scope.regs = new double[prg->n_regs];
}

void FreeTemps(Prog* prg)
{

	if (prg->n_regs > 0)
		delete[] prg->scope.regs;

}
#endif

#define encode_op(o1, o2, o3, o4) ((o4 << 16) | (o3 << 12) | (o2 << 8) | o1)


void init_code()
{
	
	func_tab[0] = &prg_main;
	func_tab[1] = &fib;

	fib.narg_in = 1;
	fib.narg_out = 1;
	fib.n_temps = 4;
	fib.n_const = 2;
	fib.const_val[0] = 1;//c0
	fib.const_val[1] = 2;//c1
	prg_main.narg_in = 0;
	prg_main.narg_out = 0;
	prg_main.n_temps = 1;
	prg_main.n_const = 1;
	prg_main.const_val[0] = 36;//c0


	prg_main.instrs[0] = { encode_op(F_CALL,T_FUNC,T_CONST,T_TEMP),{1,0,0} };		// t0 = func_tab[1] (c0)==> t0 = fib(c0)
	prg_main.instrs[1] = { encode_op(F_PRI,T_TEMP,0,0),{0,0,0} };				// print t0
	prg_main.prg_len = 2;

	// y= fib(n)
	fib.instrs[0] = { encode_op(F_LT,T_ARG_IN,T_CONST,T_TEMP),{0,1,0} };			// t0 = (n < c1)
	fib.instrs[1] = { encode_op(F_JMPNF,T_TEMP,0,0),{0,4,0} };				// if (!t0) goto instruction 4
	fib.instrs[2] = { encode_op(F_SET,T_ARG_OUT,T_ARG_IN,0), {0,0,0 } };			// y=n
	fib.instrs[3] = { encode_op(F_RET,0,0,0), {0,0,0 } };					// return
	fib.instrs[4] = { encode_op(F_SUB,T_ARG_IN,T_CONST,T_TEMP),{0,0,0} };			// t0 = n - c0
	fib.instrs[5] = { encode_op(F_CALL,0,T_TEMP,T_TEMP),{1,0,1} };				// t1 = fib(t0)
	fib.instrs[6] = { encode_op(F_SUB,T_ARG_IN,T_CONST,T_TEMP),{0,1,2} };			// t2 = n-c1
	fib.instrs[7] = { encode_op(F_CALL,0,T_TEMP,T_TEMP),{1,2, 3 } };			// t3= fib(t2)
	fib.instrs[8] = { encode_op(F_ADD,T_ARG_OUT,T_TEMP,T_TEMP),{0,1,3} };			// y= t1+t3
	fib.prg_len = 9;
}

// Access to register i of the current instruction
#define ARG_I(i) curr_instr.args[i]

// temporary variables for each scope
#define TEMP prg->scope.regs

// constant values of the current function
#define CONST prg->const_val

// Execute program
// Prg = the program to be executed 
// IN : pointer to inpout argument
// OUT : pointer to function output

void exec(Prog* prg,double *IN,double *OUT)
{
	size_t ip = 0;
	Scope old_scope = prg->scope;
	
	AllocTemps(prg);
	while (ip < prg->prg_len)
	{
		Instr curr_instr = prg->instrs[ip++];
		switch (curr_instr.op)
		{
		
		case encode_op(F_SET, T_ARG_OUT, T_ARG_IN, 0):
			
			*OUT= *IN;
			MYDEBUG(cout << "F_SET_O_I : " << *OUT << " = " << *IN << endl;)
			break;

		case encode_op(F_LT, T_ARG_IN, T_CONST, T_TEMP):
			TEMP[ARG_I(2)] = *IN < CONST[ARG_I(1)];
			MYDEBUG(cout << "F_LT_I_C_T : " << TEMP[ARG_I(2)] << " = ( " << *IN << " < " << CONST[ARG_I(1)] << " )" <<endl;)
			break;

		case  encode_op(F_JMPNF, T_TEMP, 0, 0):
			MYDEBUG(cout << "F_JMPNF_T : if ! " << TEMP[ARG_I(0)] << " goto  " << (int)ARG_I(1)  << endl;)
			if (!TEMP[ARG_I(0)])
				ip = ARG_I(1);
			break;

		case F_RET:
			MYDEBUG(cout << "F_RET : goto  " << prg->prg_len  << endl;)
			ip = prg->prg_len;
			break;


		case encode_op(F_ADD, T_ARG_OUT, T_TEMP, T_TEMP):
			
			*OUT = TEMP[ARG_I(1)]+ TEMP[ARG_I(2)];
			MYDEBUG(cout << "F_ADD_O_T_T : " << *OUT << " = " << TEMP[ARG_I(1)] << " + " << TEMP[ARG_I(2)] << endl;)
			break;

		case encode_op(F_SUB, T_ARG_IN, T_CONST, T_TEMP):
			TEMP[ARG_I(2)] = *IN - CONST[ARG_I(1)];
			MYDEBUG(cout << "F_SUB_I_C_T : " << TEMP[ARG_I(2)] << " = " << *IN << " - " << CONST[ARG_I(1)] << endl;)
			break;

		case encode_op(F_PRI, T_TEMP, 0, 0):
			MYDEBUG(cout << "F_PRINT : " << TEMP[ARG_I(0)] << endl;)
			{
				double r = TEMP[ARG_I(0)];
				if (((long)r) == r)
					cout << (long)r << endl;
				else
					cout << r << endl;
			}
			break;

		case  encode_op(F_CALL, 0, T_TEMP, T_TEMP):
			{	
			MYDEBUG(cout << "F_CALL_T_T: func_tab( " << (int)ARG_I(0) << ") =f(" << TEMP[ARG_I(1)] << ")" << endl;)
			exec(func_tab[ARG_I(0)], &TEMP[ARG_I(1)], &TEMP[ARG_I(2)]);
			MYDEBUG(cout << "Result = ( " << TEMP[ARG_I(2)] << ")" << endl;)
			}
			break;
		case encode_op(F_CALL, T_FUNC, T_CONST, T_TEMP):
		{
			MYDEBUG(cout << "F_CALL_C_T: func_tab( " << (int)ARG_I(0) << ") =f(" << CONST[ARG_I(1)] << ")" << endl;)
			exec(func_tab[ARG_I(0)], &CONST[ARG_I(1)], &TEMP[ARG_I(2)]);
			MYDEBUG(cout << "Result = ( " << TEMP[ARG_I(2)]  << ")" << endl;)
		}
		break;
		default:
			cout << "Illegal operation code : " << curr_instr.op << endl;
			return;
			//break;
		}
	}

	FreeTemps(prg);
	prg->scope = old_scope;
}
int main()
{
	
	clock_t start, end; 
	init_code();
#ifdef USE_POOL
	TempPool.resize(1000);
#endif
	start = clock();
	exec(&prg_main,NULL,NULL);
	end = clock();
	double t = (end - start) * 1.0 / CLOCKS_PER_SEC;
	cout << "Time = " << t << endl;

}

