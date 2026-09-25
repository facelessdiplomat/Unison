int unisonThrowProbe(int value)
{
    if (value < 0)
    {
        throw value;
    }

    return value;
}
