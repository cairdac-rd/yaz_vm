#include <iostream>
#include <vector>
#include <time.h>

using namespace std;

//#define MYDEBUG(x) x
#define MYDEBUG(x) 

#define DATA_TYPE double


//operation type
enum OP { F_SET, F_SUB, F_ADD, F_JMPNF, F_RET, F_LT, F_CALL, F_PRI };
//Argument type
enum VAR_TYPE { T_CONST = 1, T_TEMP, T_ARG_IN, T_ARG_OUT, T_GLOB_VAR, T_STATIC, T_FUNC };

struct Prog;

typedef unsigned char reg_;
struct Instr
{
	size_t op;	// op code
	size_t args[3];
};

struct Context
{
	DATA_TYPE* regs;
};
struct Prog
{
	std::vector<Instr> instrs;
	std::vector<DATA_TYPE> Static_vars;
	std::vector<DATA_TYPE> const_val;
	reg_  narg_in;
	reg_  narg_out;
	reg_  n_temps;
	Context ctx;
};

Prog prg_main;
Prog fib;

// function table
Prog* func_tab[5];


#define USE_POOL 1
#ifdef USE_POOL

//Context Temp variables pool
std::vector<DATA_TYPE> TempPool;
size_t pool_p = 0;

void AllocTemps(Prog* prg)
{
	prg->ctx = { 0 };

	if (prg->n_temps > 0)
	{
		prg->ctx.regs = &TempPool[pool_p];
		pool_p += prg->n_temps;
	}
}
void FreeTemps(Prog* prg)
{
	if (prg->n_temps > 0)
	{
		pool_p -= prg->n_temps;
	}
}
#else
void AllocTemps(Prog* prg)
{
	prg->ctx = {};
	if (prg->n_regs > 0)
		prg->ctx.regs = new DATA_TYPE[prg->n_temps];
}

void FreeTemps(Prog* prg)
{

	if (prg->n_temps > 0)
		delete[] prg->ctx.regs;

}
#endif

#define encode_op(o1, o2, o3, o4) ((o4 << 16) | (o3 << 12) | (o2 << 8) | o1)


void init_code()
{

	func_tab[0] = &prg_main;
	func_tab[1] = &fib;

	fib.narg_in = 1;
	fib.narg_out = 1;
	fib.n_temps = 2;
	fib.const_val.push_back(1);//c0
	fib.const_val.push_back(2);//c1
	prg_main.narg_in = 0;
	prg_main.narg_out = 0;
	prg_main.n_temps = 1;
	prg_main.const_val.push_back(36);//c0


	prg_main.instrs.push_back({ encode_op(F_CALL,T_FUNC,T_CONST,T_TEMP),{1,0,0} });		//(1) t0 = func_tab[1](c0) => t0 = fib(c0)
	prg_main.instrs.push_back({ encode_op(F_PRI,T_TEMP,0,0),{0,0,0} });			//(2) print t0

	// y= fib(n)
	fib.instrs.push_back({ encode_op(F_LT,T_ARG_IN,T_CONST,T_TEMP),{0,1,0} });		//(0) t0 = (n < c1)
	fib.instrs.push_back({ encode_op(F_JMPNF,T_TEMP,0,0),{0,4,0} });			//(1) if (!t0) goto instruction 4
	fib.instrs.push_back({ encode_op(F_SET,T_ARG_OUT,T_ARG_IN,0), {0,0,0} });		//(2) y=n
	fib.instrs.push_back({ encode_op(F_RET,0,0,0), {0,0,0} });				//(3) return
	fib.instrs.push_back({ encode_op(F_SUB,T_ARG_IN,T_CONST,T_TEMP),{0,0,0} });		//(4) t0 = n - c0
	fib.instrs.push_back({ encode_op(F_CALL,T_FUNC,T_TEMP,T_TEMP),{1,0,0} });		//(5) t0 = fib(t0)
	fib.instrs.push_back({ encode_op(F_SUB,T_ARG_IN,T_CONST,T_TEMP),{0,1,1} });		//(6) t1 = n-c1
	fib.instrs.push_back({ encode_op(F_CALL,T_FUNC,T_TEMP,T_TEMP),{1,1,1} });		//(7) t1= fib(t1)
	fib.instrs.push_back({ encode_op(F_ADD,T_ARG_OUT,T_TEMP,T_TEMP),{0,0,1} });		//(8) y= t0+t1

}

// Access to register i of the current instruction
#define ARG_I(i) curr_instr.args[i]

// temporary variables for each ctx
#define TEMP prg->ctx.regs

// constant values of the current function
#define CONST prg->const_val

// Execute program
// prg = pointer to the program to be executed 
// IN : pointer to inpout argument
// OUT : pointer to function output

void exec(Prog* prg, DATA_TYPE* IN, DATA_TYPE* OUT)
{
	size_t ip = 0;
	Context old_scope = prg->ctx;

	AllocTemps(prg);
	size_t N = prg->instrs.size();
	while (ip < N)
	{
		Instr curr_instr = prg->instrs[ip++];
		switch (curr_instr.op)
		{
		//F_SET
		case encode_op(F_SET, T_ARG_OUT, T_ARG_IN, 0):
			*OUT = *IN;
			MYDEBUG(cout << "F_SET_O_I : " << *OUT << " = " << *IN << endl;)
			break;
		
		case encode_op(F_SET, T_TEMP, T_ARG_IN, 0):
			TEMP[ARG_I(0)] = *IN;
			MYDEBUG(cout << "F_SET_T_I : " << TEMP[ARG_I(0)] << " = " << *IN << endl;)
				break;
		
		case encode_op(F_SET, T_TEMP, T_TEMP, 0):
			TEMP[ARG_I(0)] = TEMP[ARG_I(1)];
			MYDEBUG(cout << "F_SET_T_T : " << TEMP[ARG_I(0)] << " = " << TEMP[ARG_I(1)] << endl;)
				break;
		
		case encode_op(F_SET, T_TEMP, T_CONST, 0):
			TEMP[ARG_I(0)] = CONST[ARG_I(1)];
			MYDEBUG(cout << "F_SET_T_C : " << TEMP[ARG_I(0)] << " = " << CONST[ARG_I(1)] << endl;)
				break;
		
		case encode_op(F_SET, T_ARG_OUT, T_CONST, 0):

			*OUT = CONST[ARG_I(1)];
			MYDEBUG(cout << "F_SET_O_C : " << *OUT << " = " << CONST[ARG_I(1)] << endl;)
				break;
		
		//F_LT
		case encode_op(F_LT, T_ARG_IN, T_CONST, T_TEMP):
			TEMP[ARG_I(2)] = *IN < CONST[ARG_I(1)];
			MYDEBUG(cout << "F_LT_I_C_T : " << TEMP[ARG_I(2)] << " = ( " << *IN << " < " << CONST[ARG_I(1)] << " )" << endl;)
				break;

		case encode_op(F_LT, T_ARG_IN, T_TEMP, T_TEMP):
			TEMP[ARG_I(2)] = *IN < TEMP[ARG_I(1)];
			MYDEBUG(cout << "F_LT_I_T_T : " << TEMP[ARG_I(2)] << " = ( " << *IN << " < " << CONST[ARG_I(1)] << " )" << endl;)
				break;

		case encode_op(F_LT, T_TEMP, T_CONST, T_TEMP):
			TEMP[ARG_I(2)] = TEMP[ARG_I(0)] < CONST[ARG_I(1)];
			MYDEBUG(cout << "F_LT_T_C_T : " << TEMP[ARG_I(2)] << " = ( " << TEMP[ARG_I(0)] << " < " << CONST[ARG_I(1)] << " )" << endl;)
				break;

		case  encode_op(F_JMPNF, T_TEMP, 0, 0):
			MYDEBUG(cout << "F_JMPNF_T : if ! " << TEMP[ARG_I(0)] << " goto  " << (int)ARG_I(1) << endl;)
				if (!TEMP[ARG_I(0)])
					ip = ARG_I(1);
			break;

		case F_RET:
			MYDEBUG(cout << "F_RET : goto  " << N << endl;)
				ip = N;
			break;


		case encode_op(F_ADD, T_ARG_OUT, T_TEMP, T_TEMP):

			*OUT = TEMP[ARG_I(1)] + TEMP[ARG_I(2)];
			MYDEBUG(cout << "F_ADD_O_T_T : " << *OUT << " = " << TEMP[ARG_I(1)] << " + " << TEMP[ARG_I(2)] << endl;)
				break;

		case encode_op(F_SUB, T_ARG_IN, T_CONST, T_TEMP):
			TEMP[ARG_I(2)] = *IN - CONST[ARG_I(1)];
			MYDEBUG(cout << "F_SUB_I_C_T : " << TEMP[ARG_I(2)] << " = " << *IN << " - " << CONST[ARG_I(1)] << endl;)
				break;

		case encode_op(F_PRI, T_TEMP, 0, 0):
			MYDEBUG(cout << "F_PRINT : " << TEMP[ARG_I(0)] << endl;)
		{
			DATA_TYPE r = TEMP[ARG_I(0)];
			if (((long)r) == r)
				cout << (long)r << endl;
			else
				cout << r << endl;
		}
			break;

		case  encode_op(F_CALL, T_FUNC, T_TEMP, T_TEMP):
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
			MYDEBUG(cout << "Result = ( " << TEMP[ARG_I(2)] << ")" << endl;)
		}
		break;
		default:
			cout << "Illegal operation code : " << curr_instr.op << endl;
			return;

		}
	}

	FreeTemps(prg);
	prg->ctx = old_scope;
}

int main()
{

	clock_t start, end;
	init_code();
#ifdef USE_POOL
	TempPool.resize(1000);
#endif
	start = clock();
	exec(&prg_main, NULL, NULL);
	end = clock();
	double t = (end - start) * 1.0 / CLOCKS_PER_SEC;
	cout << "Time = " << t << endl;

}
