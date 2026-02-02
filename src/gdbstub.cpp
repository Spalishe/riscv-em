/*
Copyright 2026 Spalishe

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

*/


#include "../include/gdbstub.hpp"
#include "../include/devices/plic.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <format>
#include <sstream>
#include <algorithm>

using namespace std;
#pragma error disable 

uint32_t gdb_socket;
uint32_t gdb_recv;
sockaddr_in gdb_address;
bool gdb_isR;
HART* gdb_hart;
Machine* gdb_cpu;
atomic<bool> gdb_exec = true;
atomic<bool> gdb_sigint = false;
std::vector<uint64_t> gdb_bp;

thread gdb_execth;

string gdb_xml;

static string to_little_endian_hex(uint64_t value)
{
  ostringstream oss;
  for(int i = 0; i < 8; i++)
  {
    uint8_t byte = (value >> (i * 8)) & 0xFF;
    oss << format("{:02x}", byte);
  }
  return oss.str();
}
static std::string swapHexEndian(std::string value, int size_bytes)
{
  ostringstream oss;
  for(int i = size_bytes; i >= 0; i--)
  {
    oss << value.substr(i * 2, 2);
  }
  return oss.str();
}

vector<tuple<string,uint32_t,char,optional<vector<tuple<string,uint8_t,uint8_t>>>>> xml_data{
    {"zero", 0, 'g', nullopt},
    {"ra", 1, 'g', nullopt},
    {"sp", 2, 'g', nullopt},
    {"gp", 3, 'g', nullopt},
    {"tp", 4, 'g', nullopt},
    {"t0", 5, 'g', nullopt},
    {"t1", 6, 'g', nullopt},
    {"t2", 7, 'g', nullopt},
    {"fp", 8, 'g', nullopt},
    {"s1", 9, 'g', nullopt},
    {"a0", 10, 'g', nullopt},
    {"a1", 11, 'g', nullopt},
    {"a2", 12, 'g', nullopt},
    {"a3", 13, 'g', nullopt},
    {"a4", 14, 'g', nullopt},
    {"a5", 15, 'g', nullopt},
    {"a6", 16, 'g', nullopt},
    {"a7", 17, 'g', nullopt},
    {"s2", 18, 'g', nullopt},
    {"s3", 19, 'g', nullopt},
    {"s4", 20, 'g', nullopt},
    {"s5", 21, 'g', nullopt},
    {"s6", 22, 'g', nullopt},
    {"s7", 23, 'g', nullopt},
    {"s8", 24, 'g', nullopt},
    {"s9", 25, 'g', nullopt},
    {"s10", 26, 'g', nullopt},
    {"s11", 27, 'g', nullopt},
    {"t3", 28, 'g', nullopt},
    {"t4", 29, 'g', nullopt},
    {"t5", 30, 'g', nullopt},
    {"t6", 31, 'g', nullopt},
    {"pc", 32, 'g', nullopt},
    {"mstatus", MSTATUS, 'c', vector<tuple<string,uint8_t,uint8_t>>{
        {"SIE", 1, 1},
        {"MIE", 3, 3},
        {"SPIE", 5, 5},
        {"UBE", 6, 6},
        {"MPIE", 7, 7},
        {"SPP", 8, 8},
        {"VS", 9, 10},
        {"MPP", 11, 12},
        {"FS", 13, 14},
        {"XS", 15, 16},
        {"MPRV", 17, 17},
        {"TVM", 20, 20},
        {"TW", 21, 21},
        {"TSR", 22, 22},
    }},
    {"cycle", CYCLE, 'c', nullopt},
    {"time", TIME, 'c', nullopt},
    {"instret", INSTRET, 'c', nullopt},
    {"misa", MISA, 'c', nullopt},
    {"medeleg", MEDELEG, 'c', nullopt},
    {"mideleg", MIDELEG, 'c', nullopt},
    {"mie", MIE, 'c', nullopt},
    {"mip", MIP, 'c', nullopt},
    {"mtvec", MTVEC, 'c', nullopt},
    {"mcounteren", MCOUNTEREN, 'c', nullopt},
    {"mscratch", MSCRATCH, 'c', nullopt},
    {"mhartid", MHARTID, 'c', nullopt},
    {"mepc", MEPC, 'c', nullopt},
    {"mcause", MCAUSE, 'c', nullopt},
    {"mtval", MTVAL, 'c', nullopt},
    {"sstatus", SSTATUS, 'c', vector<tuple<string,uint8_t,uint8_t>>{
        {"SIE", 1, 1},
        {"SPIE", 5, 5},
        {"UBE", 6, 6},
        {"SPP", 8, 8},
        {"VS", 9, 10},
        {"FS", 13, 14},
        {"XS", 15, 16},
    }},
    {"sedeleg", SEDELEG, 'c', nullopt},
    {"sideleg", SIDELEG, 'c', nullopt},
    {"sie", SIE, 'c', nullopt},
    {"sip", SIP, 'c', nullopt},
    {"stvec", STVEC, 'c', nullopt},
    {"scounteren", SCOUNTEREN, 'c', nullopt},
    {"sscratch", SSCRATCH, 'c', nullopt},
    {"sepc", SEPC, 'c', nullopt},
    {"scause", SCAUSE, 'c', nullopt},
    {"stval", STVAL, 'c', nullopt},
    {"stimecmp", STIMECMP, 'c', nullopt},
    {"satp", SATP, 'c', vector<tuple<string,uint8_t,uint8_t>>{
        {"MODE",SATP_MODE_LOW,SATP_MODE_HIGH},
        {"ASID",SATP_ASID_LOW,SATP_ASID_HIGH},
        {"PPN",SATP_PPN_LOW,SATP_PPN_HIGH},
    }},
    {"priv", 0, 'v', nullopt},
};

string GDB_CreateXML() {
    std::ostringstream output;
    output <<
        R"(<?xml version="1.0"?>
    <!DOCTYPE target SYSTEM "gdb-target.dtd">
    <target version="1.0">
    <architecture>riscv:rv64</architecture>
    <feature name="org.gnu.gdb.riscv.cpu">)"
            << endl;

    for(uint64_t i = 0; i < xml_data.size(); i++)
    {
        auto cur_reg = xml_data.at(i);
        if(get<2>(cur_reg) == 'g')
        {
            output << format(R"(    <reg name="{}" bitsize="{}" regnum="{}")", get<0>(cur_reg), 64, get<1>(cur_reg));
            if(get<3>(cur_reg).has_value()) {
                output << ">" << endl;
                for(auto &dat : *get<3>(cur_reg)) {
                    output << format(R"(      <field name="{}" start="{}" end="{}"/>)", get<0>(dat), get<1>(dat), get<2>(dat)) << endl;
                }
                output << "    </reg>" << endl;
            } else {
                output << "/>" << endl;
            }
        }
    }
    output << "  </feature>" << endl;
    output << R"(  <feature name="org.gnu.gdb.riscv.virtual">)" << endl;
    for(uint64_t i = 0; i < xml_data.size(); i++)
    {
        auto cur_reg = xml_data.at(i);
        if(get<2>(cur_reg) == 'v')
        {
            output << format(R"(    <reg name="{}" bitsize="{}")", get<0>(cur_reg), 64);
            if(get<3>(cur_reg).has_value()) {
                output << ">" << endl;
                for(auto &dat : *get<3>(cur_reg)) {
                    output << format(R"(      <field name="{}" start="{}" end="{}"/>)", get<0>(dat), get<1>(dat), get<2>(dat)) << endl;
                }
                output << "    </reg>" << endl;
            } else {
                output << "/>" << endl;
            }
        }
    }
    output << "  </feature>" << endl;
    output << R"(  <feature name="org.gnu.gdb.riscv.csr">)" << endl;
    for(uint64_t i = 0; i < xml_data.size(); i++)
    {
        auto cur_reg = xml_data.at(i);
        if(get<2>(cur_reg) == 'c')
        {
            output << format(R"(    <reg name="{}" bitsize="{}" regnum="{}")", get<0>(cur_reg), 64, get<1>(cur_reg));
            if(get<3>(cur_reg).has_value()) {
                output << ">" << endl;
                for(auto &dat : *get<3>(cur_reg)) {
                    output << format(R"(      <field name="{}" start="{}" end="{}"/>)", get<0>(dat), get<1>(dat), get<2>(dat)) << endl;
                }
                output << "    </reg>" << endl;
            } else {
                output << "/>" << endl;
            }
        }
    }
    output << "  </feature>" << endl;
    output << "</target>" << endl;
    return output.str();
}

void GDB_Create(HART* hart, Machine* cpu) {
    gdb_socket = socket(AF_INET, SOCK_STREAM, 0);
    int op = 1;
    setsockopt(gdb_socket,SOL_SOCKET, SO_REUSEADDR, &op, sizeof(op));
    gdb_hart = hart;
    gdb_cpu = cpu;

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(1512);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // binding socket.
    int t = bind(gdb_socket, (struct sockaddr*)&serverAddress,
         sizeof(serverAddress));

    if(t < 0) {
        perror("[GDB] Bind failed! ");
        close(gdb_socket);
        return;
    }
    cout << "[GDB] Stub created on port 1512" << endl;

    gdb_isR = true;
    GDB_Loop();
}

void GDB_Stop() {
    gdb_isR = false;
    cout << "[GDB] Disconnecting client" << endl;
    close(gdb_recv);
    close(gdb_socket);
}

uint32_t GDB_send(string buffer, uint32_t flags = 0) {
    size_t total = 0;
    while (total < buffer.size() ) {
        ssize_t sent = send(gdb_recv, buffer.c_str() + total, buffer.size() - total, flags);
        if (sent <= 0) return sent; // error or disconnect
        total += sent;
    }
    return total;
}
uint32_t GDB_sendPacket(string buffer, uint32_t flags = 0) {
    uint8_t sum = 0;
    for(char &ch : buffer) {
        sum += ch;
    }
    buffer = format("${:}#{:02x}\0",buffer,sum);
    size_t total = 0;
    while (total < buffer.size() ) {
        ssize_t sent = send(gdb_recv, buffer.c_str() + total, buffer.size() - total, flags);
        if (sent <= 0) return sent; // error or disconnect
        total += sent;
    }
    return total;
}
string GDB_unformatPacket(string buffer) {
    buffer = buffer.substr(1);
    return buffer.substr(0,buffer.size()-3);
}

void GDB_parsePacket(const char* buffer) {
    if(buffer[0] == '$') {
        string packet = GDB_unformatPacket(string(buffer));
        if(packet.starts_with("qSupported:")) {
            GDB_sendPacket("PacketSize=4096;qXfer:features:read+;vContSupported+;swbreak+;hwbreak+;");
            return;
        }
        if(packet.starts_with("qXfer:features:read:target.xml:")) {
            if(gdb_xml.size() == 0)
                gdb_xml = GDB_CreateXML();

            string dat = packet.substr(31);
            uint64_t startp = stoul(dat.substr(0,dat.find(',')),nullptr,16);
            uint64_t endp = stoul(dat.substr(dat.find(',')+1),nullptr,16);
            
            GDB_sendPacket(((startp + endp) >= gdb_xml.size() ? "l" : "m") + gdb_xml.substr(startp,endp));
            return;
        }
        if(packet.starts_with("vCont?")) {
            GDB_sendPacket("vCont;s;S;c;C");
            return;
        }
        if(packet.starts_with("vCont;")) {
            std::string args = packet.substr(6);
            if(args.size() == 0)
            {
                return;
            }
            switch(args.at(0))
            {
                case 'c': {
                    gdb_exec = true;
                    gdb_execth = thread([]() {
                        while(gdb_exec) {
                            if(!gdb_cpu->gdb_single_step) {
                                uint64_t pc = gdb_hart->pc;
                                auto it = find(gdb_bp.begin(), gdb_bp.end(), gdb_hart->pc);
                                if(it != gdb_bp.end()) {
                                    gdb_exec = false;
                                    break;
                                }
                                gdb_cpu->gdb_single_step = true;
                            }
                        }
                        GDB_sendPacket(gdb_sigint ? "S02" : "S05");
                        gdb_sigint = false;
                    });
                    gdb_execth.detach();

                    break;
                }
                case 's':
                    gdb_cpu->gdb_single_step = true;
                    GDB_sendPacket("S05");
                    break;
                default:
                    GDB_sendPacket("");
                    break;
            }
            return;
        }
        if(packet.starts_with("k")) {
            gdb_isR = false;
            std::cout << "[GDB] Killed by GDB stub" << std::endl;
            gdb_cpu->state = MachineState::PoweringOff;
            return;
        }
        if(packet.starts_with("c")) {
            gdb_exec = true;
            gdb_execth = thread([]() {
                while(gdb_exec) {
                    if(!gdb_cpu->gdb_single_step) {
                        uint64_t pc = gdb_hart->pc;
                        auto it = find(gdb_bp.begin(), gdb_bp.end(), gdb_hart->pc);
                        if(it != gdb_bp.end()) {
                            gdb_exec = false;
                            break;
                        }
                        gdb_cpu->gdb_single_step = true;
                    }
                }
                GDB_sendPacket(gdb_sigint ? "S02" : "S05");
                gdb_sigint = false;
            });
            gdb_execth.detach();
            return;
        }
        if(packet.starts_with("s")) {
            gdb_cpu->gdb_single_step = true;
            GDB_sendPacket("S05");
            return;
        }
        if(packet.starts_with("Hg")) {
            // Switch to thread(actually reset all)
        }
        if(packet.starts_with("?")) {
            // Trap reason
            GDB_sendPacket(format("S{:02d}",gdb_hart->csrs[MCAUSE]));
            return;
        }
        if(packet.starts_with("qfThreadInfo")) {
            GDB_sendPacket("m1");
            return;
        }
        if(packet.starts_with("Hc")) {
            GDB_sendPacket("OK");
            return;
        }
        if(packet.starts_with("qC")) {
            GDB_sendPacket(format("QC{:x}",1));
            return;
        }
        if(packet.starts_with("qAttached")) {
            GDB_sendPacket("0");
            return;
        }
        if(packet.starts_with("p")) {
            //xml get
            uint64_t idx = stoul(packet.substr(1),nullptr,16);
            if(idx <= 31) {
                //gpr
                GDB_sendPacket(to_little_endian_hex(gdb_hart->GPR[idx]));
            } else if(idx == 32) {
                //pc
                GDB_sendPacket(to_little_endian_hex(gdb_hart->pc));
            } else if(idx > 32 && idx < 64) {
                if(idx == 33) {
                    //priv
                    GDB_sendPacket(to_little_endian_hex((uint8_t)gdb_hart->mode));
                }
            } else if(idx >= 64) {
                //csr
                GDB_sendPacket(to_little_endian_hex(csr_read(gdb_hart,idx)));
            }
            return;
        }
        if(packet.starts_with("P")) {
            //xml get
            uint64_t idx = stoul(packet.substr(1),nullptr,16);
            string data = packet.substr(packet.find('=') + 1);
            uint64_t num = 0;
            for(uint64_t i = 0; i < 8; i++) {
                num = num | (stoul(data.substr(i*2,2),nullptr,16) << i);
            }
            if(idx <= 31) {
                //gpr
                gdb_hart->GPR[idx] = num;
            } else if(idx == 32) {
                //pc
                gdb_hart->pc = num;
            } else if(idx > 32 && idx < 64) {
                if(idx == 33) {
                    //priv
                    gdb_hart->mode = (PrivilegeMode)num;
                }
            } else if(idx >= 64) {
                //csr
                csr_write(gdb_hart,idx,num);
            }
            GDB_sendPacket("OK");
            return;
        }
        if(packet.starts_with("g")) {
            //registers
            string packet;
            for(int i = 0; i < 32; i++) {
                packet += to_little_endian_hex(gdb_hart->GPR[i]);
            }
            //pc
            packet += to_little_endian_hex(gdb_hart->pc);
            GDB_sendPacket(packet);
            return;
        }
        if(packet.starts_with("G")) {
            //set registers
            std::string data = packet.substr(1);
            // Set GPR & PC
            // GPR
            uint64_t val = 0;
            for(uint64_t i = 0; i < 32; i++)
            {
                val = stoull(swapHexEndian(data.substr(i * 16, 16), 8),
                            nullptr, 16);
                gdb_hart->GPR[i] = val;
            }

            val = stoull(
                swapHexEndian(
                    data.substr(32 * 16, 16),
                    8),
                nullptr, 16);

            gdb_hart->pc = val;
            GDB_sendPacket("OK");
            return;
        }
        if(packet.starts_with("M")) {
            //write mem
            uint64_t address = stoul(packet.substr(1, packet.find(',')),nullptr,16);
            uint64_t size = stoul(packet.substr(packet.find(',')+1, packet.find(':',packet.find(',')+1)),nullptr,16);
            string data = packet.substr(packet.find(':') + 1);
            
            for(uint64_t i = 0; i < size; i++) {
                gdb_hart->mmio->store_GDB(gdb_hart,address+i,8,stoul(data.substr(i*2,2),nullptr,16));
            }

            GDB_sendPacket(format("{:x}",size));
            return;
        }
        if(packet.starts_with("m")) {
            //read mem
            uint64_t address = stoul(packet.substr(1, packet.find(',')),nullptr,16);
            uint64_t size = stoul(packet.substr(packet.find(',')+1),nullptr,16);

            string resp;

            for(uint64_t i = 0; i < size; i++) {
                optional<uint64_t> val = gdb_hart->mmio->load_GDB(gdb_hart,address+i,8);
                if(!val.has_value()) {
                    GDB_sendPacket("E01");
                    return;
                }
                resp += format("{:02x}", *val);
            }

            GDB_sendPacket(resp);
            return;
        }
        if(packet.starts_with("Z0")) {
            string dat = packet.substr(packet.find(',')+1);
            uint64_t addr = stoul(dat.substr(0,dat.find(',')),nullptr,16);
            uint64_t kind = stoul(dat.substr(dat.find(',')+1),nullptr,16);
            //optional<uint64_t> inst = memmap.load_safe(addr,32);
            //if(!inst.has_value()) {
            //    GDB_sendPacket("E01");
            //    return;
            //}
            if(find(gdb_bp.begin(), gdb_bp.end(), addr) != gdb_bp.end()) {
                GDB_sendPacket("E01");
                return;
            }
            gdb_bp.push_back(addr);
            GDB_sendPacket("OK");
            return;
        }
        if(packet.starts_with("z0")) {
            string dat = packet.substr(packet.find(',')+1);
            uint64_t addr = stoul(dat.substr(0,dat.find(',')),nullptr,16);
            uint64_t kind = stoul(dat.substr(dat.find(',')+1),nullptr,16);
            //optional<uint64_t> inst = memmap.load_safe(addr,32);
            //if(!inst.has_value()) {
            //    GDB_sendPacket("E01");
            //    return;
            //}
            if(find(gdb_bp.begin(), gdb_bp.end(), addr) == gdb_bp.end()) {
                GDB_sendPacket("E01");
                return;
            }
            gdb_bp.erase(find(gdb_bp.begin(), gdb_bp.end(), addr));
            GDB_sendPacket("OK");
            return;
        }
        if(packet.starts_with("X")) {
            
        }
        GDB_sendPacket("");
    } else {
    }
}

void GDB_Loop() {
    // listening to the assigned socket
    listen(gdb_socket, 5);

    // accepting connection request
    gdb_recv = accept(gdb_socket, nullptr, nullptr);
    int flag = 1;
    setsockopt(gdb_recv, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int));
    std::cout << "[GDB] Got connection" << std::endl;

    char buffer[4096] = { 0 };
    while(gdb_isR) {
        memset(&buffer,0,sizeof(buffer));
        int received = recv(gdb_recv, buffer, sizeof(buffer), 0);
        if(received == 0) {
            break;
        }
        if(received < 0) {
            break;
        }

        if(buffer[0] == '$') {
            GDB_send("+\0",0);
            GDB_parsePacket(buffer);
        }
        if(buffer[0] == 3) {
            //SIGINT
            GDB_send("+\0",0);
            gdb_sigint = true;
            gdb_exec = false;
        }
        if(buffer[0] == '-' || buffer[0] == '+') {
            if(buffer[1] == '$') {
                // new packet right after ack
                string str = string(buffer);
                str.erase(0, 1);
                GDB_send("+\0",0);
                GDB_parsePacket(str.c_str());
            }
        }
    }
    close(gdb_recv);
    if(gdb_isR) GDB_Loop();
}