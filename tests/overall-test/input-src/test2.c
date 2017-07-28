# 1 "test2.c"
# 1 "<built-in>"
# 1 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 1 "<command-line>" 2
# 1 "test2.c"
const char g_str1[30] = "\n \f \v string \r string \x13 \n \f";
char g_str2[30] = "\n \f \v string \r string \x13 \n \f";
char * g_ptr1 = &g_str2;

typedef struct
{
 float field1;
 float field2;
} Struct2;

typedef struct
{
 int field1;
 int field2;
 double field3;
 double field4;
 Struct2 field5;
 Struct2 field6;
 int field7[10];
 int field8[10];
} Struct1;

const Struct1 g_structInstance1;
Struct1 g_structInstance2;

int g_num1 = 0;
int * g_ptr2 = &g_num1;
const int * g_ptr3 = &g_num1;

enum g_enum
{
 VAL1,
 VAL2,
 VAL3
};

int foo1();

int main()
{
 char main_str1[30] = "\n \f \v string \r string \x13 \n \f";
 char main_str2[30] = "\n \f \v string \r string \x13 \n \f";

 const Struct1 main_structInstance1;
 Struct1 main_structInstance2;
 register Struct1 main_structInstance3;
 Struct1 *main_ptr1;
 main_ptr1 = &main_structInstance2;

 int main_num1 = !(-main_ptr1->field8[0]++) / ~((int) --main_str2[0]);

 unsigned int main_arr1[10+30];

 switch (main_num1)
 {
  case 10:
  case '\x1E':
  case 30 % 10:
   main_arr1[0] = (char) main_num1 * (float) main_num1;
   break;
 };

 while (10 <= 30 && 30 >> 0)
 {
  do
  {
   main_structInstance2 = main_structInstance1;
   main_ptr1 = (void *) main_ptr1;
  } while (g_ptr3 - g_ptr2 ||
    g_ptr3 < g_ptr2 &&
    (g_ptr3 || g_ptr2));

  for (; main_num1 + main_ptr1;)
   main_ptr1 += 10;
 }

 label1:
 return 0;
}

int foo1()
{
 const char foo1_str1[30] = "\n \f \v string \r string \x13 \n \f";
 register char foo1_str2[30] = "second string";
 char foo1_c1 = '\xffq';
 const char foo1_c2 = 'ab';

 int foo1_num1 = 30 & 0;
 int * foo1_ptr1;
 foo1_ptr1 = &foo1_num1;
 const int * foo1_ptr2 = &foo1_num1;
 register int foo1_num2 = g_num1;
 register int ** foo1_ptr3;
 int *** foo1_ptr4 = &foo1_ptr2;
 foo1_ptr3 = &(***foo1_ptr4);

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

  foo1();
 }

 enum g_enum foo1_enumVal2;
 foo1_enumVal2 = VAL3;

 goto label1; goto label2;

 label1:
 return 0;
}
