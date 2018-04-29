long map(long x, long in_min, long in_max, long out_min, long out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

extern void setup(void);
extern void loop(void);

int main()
{
   setup();
   for (;;)
     loop();
   return 0;
}
