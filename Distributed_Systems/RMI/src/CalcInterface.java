/**
 * This class provides an interface for the calculator class.
 *
 */
public interface CalcInterface {
    /**
     * Add 2 numbers
     * @param a The first number
     * @param b The second number
     * @return  The sum of the 2 numbers
     */
    double add(double a, double b);
    
    /**
     * Subtract 2 numbers
     * @param a The first number
     * @param b The second number
     * @return  The difference of the 2 numbers
     */
    double sub(double a, double b);
    
    /**
     * Multiply 2 numbers
     * @param a The first number
     * @param b The second number
     * @return  The product of the 2 numbers
     */
    double mul(double a, double b);
    
    /**
     * Divide 2 numbers
     * @param a The first number
     * @param b The second number
     * @return  The divison quotient of the 2 numbers
     */
    double div(double a, double b);
}
