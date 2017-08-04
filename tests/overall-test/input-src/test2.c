# 1 "my-test.c"
# 1 "<built-in>"
# 1 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 1 "<command-line>" 2
# 1 "my-test.c"

const char g_str1[40] = "\v global \r string \x13 \n \f";
char g_str2[40] = "\v global \r string \x13 \n \f";
char g_str3['\x1E'] = "\v global local string \x13 \n \f";


const char g_char1 = 'a';
char g_char2 = 'a';


double g_float2 = 1.5;
const float g_float3 = 2.5;


int g_num1 = 0;

int * g_ptr2 = &g_num1;
const int * g_ptr3 = &g_num1;
char * g_ptr1 = &g_str2;


void foo1();

typedef struct
{
  float field1;
  float field2;
} Struct2;


float foo2(int n1, int n2);

struct Struct1
{
  int field1;
  int field2;
  double field3;
  double field4;
  Struct2 field5;
  Struct2 field6;
  int field7[40];
  int field8[40];
};


int* foo3();

const struct Struct1 g_structInstance1;
struct Struct1 g_structInstance2;

enum g_enum
{
  VAL1,
  VAL2,
  VAL3
};


int foo4();

int main()
{

  const char main_str1[30] = "\n \f \v local \r string \x13 \n \f";


  char main_str2[30] = "\n \f \v local \r string \x13 \n \f";


  const float main_float1 = 2.5;


  float main_float2 = 3.5 / foo2(*(foo3()), foo4());


  register float main_float3 = 3.5;

  const struct Struct1 main_structInstance1;
  struct Struct1 main_structInstance2;
  register struct Struct1 main_structInstance3;

  struct Struct1 *main_ptr1;


  main_ptr1 = &((main_structInstance2));


  int main_num1 = !(-main_ptr1->field8[30 * 0]++) / ~((int) --main_str2[0]);



  unsigned int main_arr1[30 - main_num1];


  switch (main_num1)
  {

    case 0:


    case '\x1E':


    case 30 + 30:


      main_arr1[0] = (char) main_num1 * (float) main_num1;

      break;
  };


  while (0 <= 30 && 30 >> 0)
  {
    do
    {

      main_structInstance2 = main_structInstance1;


      main_ptr1 = (void *) main_ptr1;

      goto label1;


    } while (g_ptr3 - g_ptr2 ||
            g_ptr3 < g_ptr2 &&
            (g_ptr3 || g_ptr2));


    for (; main_num1 + main_ptr1;)

      main_ptr1 += 30;
  }


  foo1();


  label1:


  return foo4();
}


void foo1()
{

  register char foo1_str1[30] = "\v global local string \x13 \n \f";


  char foo1_c1 = 'ab';


  const char foo1_c2 = 'ab';


  int foo1_num1 = 30 & 0;


  register int foo1_num2 = g_num1;

  int * foo1_ptr1;


  foo1_ptr1 = &foo1_num1;


  int * const foo1_ptr2 = &g_structInstance1.field1;


  int ** foo1_ptr3 = &foo1_ptr2;

  register int * foo1_ptr4;


  foo1_ptr4 = &(**foo1_ptr3);


  goto label2; goto label1;


  if ("second string")
  {

    double foo1_num3 = g_structInstance1.field3 + g_structInstance2.field1;


    Struct2 foo1_structInstance1 = g_structInstance1.field5;


    foo1_num3 = g_structInstance2.field7[0];

    enum g_enum foo1_enumVal1;
    foo1_enumVal1 = VAL2;


    label2:


    foo1_c1 = foo1_str1[0];
  }

  enum g_enum foo1_enumVal2;
  foo1_enumVal2 = VAL3;

  goto label1; goto label2;


  label1:
  return;
}


float foo2(int n1, int n2)
{

  return 2.5;
}


int* foo3()
{

  return &g_num1;
}


int foo4()
{

  return 0;
}
