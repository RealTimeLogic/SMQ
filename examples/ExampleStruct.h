
/*
  We use these two structs when sending binary data from "publish" to
  "subscribe".
*/

struct ExampleStructA
{
   char str[10];
   int i;
   double d;
};

struct ExampleStructB
{
   ExampleStructA a;
   ExampleStructA b;
};


