package gobackn;

@license(LGPL);
//
// TODO documentation
//
network Network
{
    parameters:
        int WS = default(3);		// The sender window size
        int TO = default(10);		// The timeout interval
        double PT = default(0.5);	// The sender's and receiver's processing time
        double TD = default(1.0);	// The transmission delay
        double ED = default(4.0); 	// The error delay
        double DD = default(0.1);	// The duplication delay
        double LP = default(0.0);	// ACK/NACK's loss probability

        @display("bgb=518,279");
    submodules:
        Node0: Node {
            @display("p=91,68;i=device/pc");
        }
        Node1: Node {
            @display("p=400,68;i=device/pc");
        }
        coordinator: Coordinator {
            @display("p=255,174;i=block/cogwheel");
        }
    connections:
        Node0.port <--> Node1.port;
        coordinator.outs[0] --> Node0.coordinatorIn;
        coordinator.outs[1] --> Node1.coordinatorIn;
}

