spx_int16_t my_floor(spx_int16_t x);

spx_int16_t my_floor(spx_int16_t x)
{
   spx_int16_t y=x;
    if( (*( ( (int *) &y)+1) & 0x80000000)  != 0) //或者if(x<0)
        return (spx_int16_t)((int)x)-1;
    else
        return (spx_int16_t)((int)x);
}

