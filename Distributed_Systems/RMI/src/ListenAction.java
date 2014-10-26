import java.io.DataInputStream;
import java.io.IOException;
import java.io.ObjectOutputStream;
import java.net.Socket;


public class ListenAction extends Thread {
	/**
	 * The Socket used for communication.
	 */
	private Socket s;
	

	public void run() {
	    
	        DataInputStream in;
	        String msg = "bad";
            try {
                in = new DataInputStream(s.getInputStream());
                msg = in.readUTF();
            } catch (IOException e1) {
                e1.printStackTrace();
            }
	        
	        String[] broken = msg.split(":");
	        
		    if (broken[0].equalsIgnoreCase("add")) {

		        ObjectOutputStream  out;
		      		try {
						out = new ObjectOutputStream (s.getOutputStream());
						
						Calc temp = new Calc();
						out.writeObject(temp.add(Double.parseDouble(broken[1]), 
                                Double.parseDouble(broken[2])));
					} catch (IOException e) {
						e.printStackTrace();
					}
	  		} else if (broken[0].equalsIgnoreCase("sub")) {

                ObjectOutputStream  out;
                    try {
                        out = new ObjectOutputStream (s.getOutputStream());
                        
                        Calc temp = new Calc();
                        out.writeObject(temp.sub(Double.parseDouble(broken[1]), 
                                Double.parseDouble(broken[2])));
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
            } else if (broken[0].equalsIgnoreCase("mul")) {

                ObjectOutputStream  out;
                    try {
                        out = new ObjectOutputStream (s.getOutputStream());
                        
                        Calc temp = new Calc();
                        out.writeObject(temp.mul(Double.parseDouble(broken[1]), 
                                Double.parseDouble(broken[2])));
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
            } else if (broken[0].equalsIgnoreCase("div")) {

                ObjectOutputStream  out;
                    try {
                        out = new ObjectOutputStream (s.getOutputStream());
                        
                        Calc temp = new Calc();
                        out.writeObject(temp.div(Double.parseDouble(broken[1]), 
                                Double.parseDouble(broken[2])));
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
            } else if (broken[0].equalsIgnoreCase("upper")) {

                ObjectOutputStream  out;
                    try {
                        out = new ObjectOutputStream (s.getOutputStream());
                        
                        Case temp = new Case();
                        out.writeObject(temp.upper(broken[1]));
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
            } else if (broken[0].equalsIgnoreCase("lower")) {

                ObjectOutputStream  out;
                    try {
                        out = new ObjectOutputStream (s.getOutputStream());
                        
                        Case temp = new Case();
                        out.writeObject(temp.lower(broken[1]));
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
            } else if (broken[0].equalsIgnoreCase("lookup")) {
		        RemoteObjectReference ret = null;
		        
		    	ret = RMIRegistry.registry.get(broken[1]);
		    	
		      	ObjectOutputStream out;
	      		try {
					out = new ObjectOutputStream(s.getOutputStream());
					out.writeObject(ret);
				} catch (IOException e) {
					e.printStackTrace();
				}
		    }
	}
	
	/**
	 * The constructor to fill the values.
	 * 
	 * @param s The Socket used to communication.
	 */
	public ListenAction(Socket s) {
		this.s = s;
	}

}
