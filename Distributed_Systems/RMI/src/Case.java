
/**
 * This class provides the implementation of the case functions. 
 */
public class Case implements CaseInterface{

    @Override
    public String upper(String str) {
        return str.toUpperCase();
    }

    @Override
    public String lower(String str) {
        return str.toLowerCase();
    }

}
