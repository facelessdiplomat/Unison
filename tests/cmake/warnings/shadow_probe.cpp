int sumWithShadowedTotal(int value)
{
    int total = value;

    {
        int total = 2;
        value += total;
    }

    return total + value;
}
