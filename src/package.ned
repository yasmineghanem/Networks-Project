package gobackn;

@license(LGPL);
//
// TODO documentation
//
network Network
{
    parameters:
        int WS = default(3);		// The sender window size
        double TO = default(10.0);		// The timeout interval
        double PT = default(0.5);	// The sender's and receiver's processing time
        double TD = default(1.0);	// The transmission delay
        double ED = default(4.0); 	// The error delay
        double DD = default(0.1);	// The duplication delay
        double LP = default(0.0);	// ACK/NACK's loss probability

        @display("bgb=518,279");
    submodules:
        Nodes[2]: Node {
            @display("i=device/pc");
        }
        coordinator: Coordinator {
            @display("p=272,103;i=block/cogwheel");
        }
    connections:
        Nodes[0].port <--> Nodes[1].port;
        coordinator.outs[0] --> Nodes[0].coordinatorIn;
        coordinator.outs[1] --> Nodes[1].coordinatorIn;
}

