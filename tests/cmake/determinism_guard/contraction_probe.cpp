float multiplyThenAdd(float factor, float otherFactor, float addend)
{
    return factor * otherFactor + addend;
}

int main()
{
    volatile float slightlyAboveOne = 1.0F + 0x1.0p-23F;
    volatile float negatedRoundedSquare = -(1.0F + 0x1.0p-22F);

    return multiplyThenAdd(slightlyAboveOne, slightlyAboveOne, negatedRoundedSquare) == 0.0F ? 0 : 1;
}
