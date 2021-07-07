#include "../include/BullyAlgo/Node.h"

int
main(int argc, char** argv)
{
    if (argc < 2) {
        printf("usage BuildAlgoNode.exe <PORT NUMBER>\n");
        return -1;
    }

    std::string port(argv[1]);
    Node        node(port);
    node.run();
    return 0;
}