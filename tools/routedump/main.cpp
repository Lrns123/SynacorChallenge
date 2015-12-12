#include <iostream>
#include <array>
#include <fstream>
#include <string>
#include <vector>
#include <set>

class SynacorBinary
{
    std::array<unsigned short, 32768> m_memory;
public:

    explicit SynacorBinary(const std::string &filename)
    {
        std::ifstream fi(filename, std::ios::in | std::ios::binary);
        if (!fi)
            throw std::runtime_error("Could not open binary");

        unsigned short address = 0;
        
        // TODO: Endianness
        while (fi)
            fi.read(reinterpret_cast<char *>(&m_memory[address++]), 2);
    }

    unsigned short read(unsigned short address) const
    {
        return m_memory.at(address);
    }

    std::string readString(unsigned short address) const
    {
        std::string ret;
        unsigned short len = read(address);

        ret.reserve(len);

        while (len--)
            ret += static_cast<char>(read(++address));
        
        return ret;
    }
};

class RoomTracer
{
public:
    using ExitVector = std::vector<std::pair<std::string, unsigned short>>;
    struct Room
    {
        unsigned short roomId;
        std::string roomName;
        ExitVector exits;

        explicit Room(unsigned short id)
            : roomId(id)
        {}
    };

    explicit RoomTracer(const SynacorBinary &binary)
        : m_binary(binary)
    {}

    std::vector<Room> traceRooms(unsigned short startRoomId) const
    {
        std::vector<Room> rooms;
        std::set<unsigned short> visited;

        traceRooms(startRoomId, rooms, visited);

        return rooms;
    }

private:

    const SynacorBinary &m_binary;

    std::string getRoomName(unsigned short roomId) const
    {
        return m_binary.readString(m_binary.read(roomId));
    }

    ExitVector getRoomExits(unsigned short roomId) const
    {
        ExitVector vec;
        unsigned short nameBase = m_binary.read(roomId + 2);
        unsigned short destinationBase = m_binary.read(roomId + 3);

        unsigned short n = m_binary.read(nameBase);
        if (n != m_binary.read(destinationBase))
            throw std::exception("Could not read room exits. Number of exit names and room ids mismatch.");

        for (unsigned short i = 1; i <= n; ++i)
            vec.emplace_back(m_binary.readString(m_binary.read(nameBase + i)), m_binary.read(destinationBase + i));

        return vec;
    }

    void traceRooms(unsigned short roomId, std::vector<Room> &rooms, std::set<unsigned short> &visited) const
    {
        if (visited.find(roomId) != visited.cend())
            return;

        visited.insert(roomId);

        Room room(roomId);

        room.roomName = getRoomName(roomId);
        room.exits = getRoomExits(roomId);

        rooms.push_back(room);

        for (const auto &exit : room.exits)
            traceRooms(exit.second, rooms, visited);
    }

};

int main(int argc, char **argv)
{
    std::cout << "Synacor Challenge route dumper." << std::endl;
    if (argc < 3)
    {
        std::cout << "Usage: " << argv[0] << " <binary> <start address> [output filename]" << std::endl;
        return 1;
    }

    try
    {
        SynacorBinary binary(argv[1]);
        unsigned short startAddress = std::stoi(argv[2], nullptr, 0);
        std::ofstream ofs(argc >= 4 ? argv[3] : "output", std::ios::out);
        if (!ofs)
        {
            std::cout << "Could not open output file for writing." << std::endl;
            return 1;
        }
        
        RoomTracer tracer(binary);
        auto rooms = tracer.traceRooms(startAddress);

        ofs << "digraph G {\n";


        // Add all room nodes
        for (const auto &room : rooms)
        {
            ofs << 'r' << room.roomId << " [label=<" << room.roomName << "<br/><font point-size=\"8\">" << room.roomId;
            if (room.roomId == startAddress)
                ofs << " - Start";
            ofs << "</font>>];\n";
        }

        ofs << '\n';

        // Add all edges
        for (const auto &room : rooms)
            for (const auto &edge : room.exits)
                ofs << 'r' << room.roomId << " -> r" << edge.second << " [label=\" " << edge.first << "\"];\n";

        ofs << "}\n";

        ofs.close();

        std::cout << "Routes dumped successfully." << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cout << "Exception occured: " << e.what() << std::endl;
    }

    
}
