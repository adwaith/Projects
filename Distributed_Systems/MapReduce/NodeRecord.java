import java.util.concurrent.ConcurrentHashMap;

public class NodeRecord {

	String MapOrReduce;
	
	//"Job_ID+Chunk_No", "Original/Replicated"
	static ConcurrentHashMap<String, String> chunks_allocated = 
			new ConcurrentHashMap<String, String>();
	
	public NodeRecord(String MapOrReduce) {
		// TODO Auto-generated constructor stub
		this.MapOrReduce = MapOrReduce;
	}

}
