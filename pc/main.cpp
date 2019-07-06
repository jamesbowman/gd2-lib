long map(long x, long in_min, long in_max, long out_min, long out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

extern void setup(void);
extern void loop(void);

int main(int argc, char **argv)
{
   setup();
   do
     loop();
   while (argc > 1);
   return 0;
}
