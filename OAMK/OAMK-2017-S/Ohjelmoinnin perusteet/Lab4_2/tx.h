#ifndef TX_H
#define TX_H

float tehtava3SyotaDesimaali()
{
    float result;
    scanf("%f", &result);
    return result;
}

int tehtava3SyotaKokonaisluku()
{
    int result;
    scanf("%i", &result);
    return result;
}

float tehtava3YmpyraLaskenta(float sade)
{
    return sade * 2.0f * 3.14159265359f;
}

float tehtava3nelikulmioLaskenta(float x, float y)
{
    return (x * 2.0f) + (y * 2.0f);
}

unsigned int tehtava4OnkoKarkausVuosi(unsigned int vuosi)
{
    return !(vuosi % 4) && ((vuosi % 100) || !(vuosi % 400));
}

#endif
