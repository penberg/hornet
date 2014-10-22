public class ConvertTest {
  public static void main(String[] args) {
    int intValue = 1;
    long longValue = 1;
    float floatValue = 1.0f;
    double doubleValue = 1.0;
    byte byteValue = 1;
    char charValue = 1;
    short shortValue = 1;

    intValue  = (int) longValue;
    intValue  = (int) floatValue;
    intValue  = (int) doubleValue;
    intValue  = byteValue;
    intValue  = charValue;
    intValue  = shortValue;

    longValue = intValue;
    longValue = (long) floatValue;
    longValue = (long) doubleValue;

    floatValue = intValue;
    floatValue = longValue;
    floatValue = (float) doubleValue;

    doubleValue = intValue;
    doubleValue = longValue;
    doubleValue = (double) floatValue;
  }
}
