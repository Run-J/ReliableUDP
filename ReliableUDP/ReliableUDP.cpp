/*
	Reliability and Flow Control Example
	From "Networking for Game Programmers" - http://www.gaffer.org/networking-for-game-programmers
	Author: Glenn Fiedler <gaffer@gaffer.org>
*/

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "Net.h"
#include "FileProcess.h"


//#define SHOW_ACKS

using namespace std;
using namespace net;

const int ServerPort = 30000;
const int ClientPort = 30001;
const int ProtocolId = 0x11223344;
const float DeltaTime = 1.0f / 30.0f;
const float SendRate = 1.0f / 30.0f;
const float TimeOut = 10.0f;
const int PacketSize = 256;


// Class Name: FlowControl
// Class Description:
class FlowControl
{
public:

	FlowControl()
	{
		printf("flow control initialized\n");
		Reset();
	}

	void Reset()
	{
		mode = Bad;
		penalty_time = 4.0f;
		good_conditions_time = 0.0f;
		penalty_reduction_accumulator = 0.0f;
	}

	void Update(float deltaTime, float rtt)
	{
		const float RTT_Threshold = 250.0f;

		if (mode == Good)
		{
			if (rtt > RTT_Threshold)
			{
				printf("*** dropping to bad mode ***\n");
				mode = Bad;
				if (good_conditions_time < 10.0f && penalty_time < 60.0f)
				{
					penalty_time *= 2.0f;
					if (penalty_time > 60.0f)
						penalty_time = 60.0f;
					printf("penalty time increased to %.1f\n", penalty_time);
				}
				good_conditions_time = 0.0f;
				penalty_reduction_accumulator = 0.0f;
				return;
			}

			good_conditions_time += deltaTime;
			penalty_reduction_accumulator += deltaTime;

			if (penalty_reduction_accumulator > 10.0f && penalty_time > 1.0f)
			{
				penalty_time /= 2.0f;
				if (penalty_time < 1.0f)
					penalty_time = 1.0f;
				printf("penalty time reduced to %.1f\n", penalty_time);
				penalty_reduction_accumulator = 0.0f;
			}
		}

		if (mode == Bad)
		{
			if (rtt <= RTT_Threshold)
				good_conditions_time += deltaTime;
			else
				good_conditions_time = 0.0f;

			if (good_conditions_time > penalty_time)
			{
				printf("*** upgrading to good mode ***\n");
				good_conditions_time = 0.0f;
				penalty_reduction_accumulator = 0.0f;
				mode = Good;
				return;
			}
		}
	}

	float GetSendRate()
	{
		return mode == Good ? 30.0f : 10.0f;
	}

private:

	enum Mode
	{
		Good,
		Bad
	};

	Mode mode;
	float penalty_time;
	float good_conditions_time;
	float penalty_reduction_accumulator;
};



// ----------------------------------------------

int main(int argc, char* argv[])
{
	enum Mode
	{
		Client,
		Server
	};

	// Parse command line arguments to determine mode of operation (Server or Client)
	Mode mode = Server;
	Address address;
	const char* fileName = NULL; // for file that want to transfer
	bool md5Test = false; // for test that verify file integrity function

	if (argc >= 2)
	{
		// If IP is passed, the mode is set to Client and the destination address is resolved
		int a, b, c, d;
#pragma warning(suppress : 4996)
		if (sscanf(argv[1], "%d.%d.%d.%d", &a, &b, &c, &d) == 4)
		{
			mode = Client;
			address = Address(a, b, c, d, ServerPort);
		}
		else
		{
			fprintf(stderr, "Invalid Ip Address !!!\n");
			return 1;
		}

		if (a < 0 || a > 255 || b < 0 || b > 255 || c < 0 || c > 255 || d < 0 || d > 255)
		{
			fprintf(stderr, "Out of IPv4 range !!!! Ex. 127.0.0.1\n");
			return 1;
		}


		// Check if user specify the filename
		if (argc >= 3)
		{
			// Retrieving the file from disk
			fileName = argv[2];
			printf("The file will be transfered: %s\n", fileName);

			if (argc >= 4)
			{
				md5Test = true;
				printf("**MD5 test mode enabled.\n");
			}
		}
		else
		{
			fprintf(stderr, "Please provide the filename you want to transfer !!!\n Usage: %s <IPv4> <fileName>\n", argv[0]);
			return 1;
		}
	}


	// First, Let's initialize the network socket library (call WSAStartup on Windows)
	if (!InitializeSockets())
	{
		printf("failed to initialize sockets\n");
		return 1;
	}


	// Then, Create a ReliableConnection object (ProtocolId for protocolID, Timeout for timeout)
	ReliableConnection connection(ProtocolId, TimeOut);


	// Select port based on mode
	const int port = mode == Server ? ServerPort : ClientPort;


	// Start the connection (bind port)
	if (!connection.Start(port))
	{
		printf("could not start connection on port %d\n", port);
		return 1;
	}


	// Set connection status of server and client
	if (mode == Client)
		connection.Connect(address); // Set the connection status to Connecting and store the destination server address
	else
		connection.Listen(); // Set the connection status to Listening





	bool connected = false;
	float sendAccumulator = 0.0f;
	float statsAccumulator = 0.0f;

	FlowControl flowControl;


	FileBlock fileBlock;
	int fileLoaded = -1;  // indicates if file loaddded
	int done = -1;        // indicates if file sent successfully

	// The main logic of load, send, recieve 
	while ( done != 0)
	{
		// Update flow control (adjust send rate based on RTT)
		if (connection.IsConnected())
			flowControl.Update(DeltaTime, connection.GetReliabilitySystem().GetRoundTripTime() * 1000.0f);


		const float sendRate = flowControl.GetSendRate();




		// detect changes in connection state

		if (mode == Server && connected && !connection.IsConnected())
		{
			flowControl.Reset();
			printf("reset flow control\n");
			connected = false;
		}



		if (!connected && connection.IsConnected())
		{
			printf("client connected to server\n");
			connected = true;

			// if using client mode, then load file from computer and split the whole file content into multiple blocks.
			if (mode == Client)
			{
				if (fileBlock.LoadFile(fileName) != 0)
				{
					fprintf(stderr, "Some error happen when loading file.\n");
					return -1;
				}

				fileLoaded = 0;
			}

		}

		if (!connected && connection.ConnectFailed())
		{
			printf("connection failed\n");
			break;
		}




		// Send our Packets (Meta + Blocks)

		sendAccumulator += DeltaTime;
		while (sendAccumulator > 1.0f / sendRate)
		{
			unsigned char packet[PacketSize];
			memset(packet, 0, sizeof(packet)); // Clear the buffer

			static int metaSent = -1; // metaSent flags if metadata has been sent
			static int n = 0; // n indicates current index of blocks

			if (mode == Client && fileLoaded == 0)
			{
				// send the meta packet first if haven't send the meta packet yet
				if (metaSent != 0)
				{
					printf("Sending %s, %llu bytes, %llu total slices.\n",
						fileBlock.GetMetaPacket().filename,
						(unsigned long long)fileBlock.GetMetaPacket().fileSize, // here using long long, bcoz we were using uint_64
						(unsigned long long)fileBlock.GetMetaPacket().totalBlocks);

					// Copy the entire MetaPacket (fixed 256 bytes) into a packet and send it out later
					memcpy(packet, &fileBlock.GetMetaPacket(), PacketSize);
					metaSent = 0;

				}
				else // send the BlockPacket if the meta packet has been sent
				{
					if (n < fileBlock.GetBlocks().size())
					{
						printf("Sending %d/%llu...\n",
							n + 1,
							(unsigned long long)fileBlock.GetMetaPacket().totalBlocks);

						memcpy(packet, &fileBlock.GetBlocks()[n], PacketSize);
						n++;

						// here is temprory MD5 hard code test
						if (md5Test)
						{
							packet[10] = 18; // 'R'
							packet[11] = 10; // 'J'
						}
					}
					else
					{
						// tell the user that all file content sent
						printf("Finish Sent file: %s\n", fileName);
						done = 0;
					}
				
				}

			}



			// Keep Send Heartbeat Packet while sending the file packets
			connection.SendPacket(packet, sizeof(packet));
			sendAccumulator -= 1.0f / sendRate;
		}


		// Receive the Packet
		while (true)
		{
			unsigned char packet[256];
			int bytes_read = connection.ReceivePacket(packet, sizeof(packet));
			if (bytes_read == 0)
				break;

			// only server have to execute the following things
			if (mode == Server)
			{
				if (fileBlock.FinishedReceivedAllData() != 0) 
				{
					printf("----------------------------------------------------------------\n");
					printf("Receiving data...\n");
					int ret = fileBlock.ProcessReceivedPacket(packet, bytes_read);
					if (ret != 0)
					{
						printf("Processed non-meta/block packet.\n");
					}
					printf("----------------------------------------------------------------\n");
				}
				else
				{
					printf("*****************************************************************\n");
					printf("All data received!\n");
					printf("Calculating the validation...\n");

					fileBlock.VerifyFileContent();
				}
			}
		}
		




		// show packets that were acked this frame

#ifdef SHOW_ACKS
		unsigned int* acks = NULL;
		int ack_count = 0;
		connection.GetReliabilitySystem().GetAcks(&acks, ack_count);
		if (ack_count > 0)
		{
			printf("acks: %d", acks[0]);
			for (int i = 1; i < ack_count; ++i)
				printf(",%d", acks[i]);
			printf("\n");
		}
#endif


		
		// Update connection status (timeout detection, statistics)
		connection.Update(DeltaTime);


		// show connection stats

		statsAccumulator += DeltaTime;

		while (statsAccumulator >= 0.25f && connection.IsConnected())
		{
			float rtt = connection.GetReliabilitySystem().GetRoundTripTime();

			unsigned int sent_packets = connection.GetReliabilitySystem().GetSentPackets();
			unsigned int acked_packets = connection.GetReliabilitySystem().GetAckedPackets();
			unsigned int lost_packets = connection.GetReliabilitySystem().GetLostPackets();

			float sent_bandwidth = connection.GetReliabilitySystem().GetSentBandwidth();
			float acked_bandwidth = connection.GetReliabilitySystem().GetAckedBandwidth();

			printf("rtt %.1fms, sent %d, acked %d, lost %d (%.1f%%), sent bandwidth = %.1fkbps, acked bandwidth = %.1fkbps\n",
				rtt * 1000.0f, sent_packets, acked_packets, lost_packets,
				sent_packets > 0.0f ? (float)lost_packets / (float)sent_packets * 100.0f : 0.0f,
				sent_bandwidth, acked_bandwidth);

			statsAccumulator -= 0.25f;
		}

		net::wait(DeltaTime);
	}


	// After use program, we have to shutdown sockets; releasing resources from the Winsock library
	ShutdownSockets();

	return 0;
}
