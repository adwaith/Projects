import java.util.concurrent.ConcurrentHashMap;


public class Master {

	//"IP+Port", NodeRecord
	static ConcurrentHashMap<Long, NodeRecord> masterTable = 
			new ConcurrentHashMap<Long, NodeRecord>();
	
	public Master(String IP, int Port) {
        String MapKey = IP+Integer.toString(Port);
    }
	
	public static void main(String[] args) {
		

	}

}
