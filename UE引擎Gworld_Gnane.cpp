// 阿念原作者留名 请勿删除

// Realizing the automatic acquisition of world class name matrix chain data by peaceful elites in the new season
// 实现新赛季和平精英自动获取世界类名矩阵链数据

#include <unistd.h>
#include <sys/uio.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <map>
#include <iomanip>

#ifndef SYS_process_vm_readv
  #if defined(__x86_64__)
    #define SYS_process_vm_readv 310
  #elif defined(__i386__)
    #define SYS_process_vm_readv 347
  #elif defined(__aarch64__)
    #define SYS_process_vm_readv 270
  #elif defined(__arm__)
    #define SYS_process_vm_readv 376
  #else
    #define SYS_process_vm_readv 310
  #endif
#endif

class UETerminalReader {
private:
    pid_t pid_;
    int proc_fd_;
    uintptr_t libue4_base_;
    
public:
    struct GameAddresses {
        uintptr_t libue4;
        uintptr_t gworld;
        uintptr_t gobjects;
        uintptr_t gnames;
        uintptr_t uworld;
        uintptr_t game_instance;
        uintptr_t local_players;
        uintptr_t local_player;
        uintptr_t player_controller;
        uintptr_t pawn;
        uintptr_t root_component;
        uintptr_t camera_manager;
        uintptr_t view_matrix;
        uintptr_t persistent_level;
        uintptr_t actor_array;
        uint32_t actor_count;
        
        // 自定义地址存储
        std::map<std::string, uintptr_t> custom_addresses;

        // 构造函数，初始化所有基本类型成员
        GameAddresses() {
            libue4 = 0;
            gworld = 0;
            gobjects = 0;
            gnames = 0;
            uworld = 0;
            game_instance = 0;
            local_players = 0;
            local_player = 0;
            player_controller = 0;
            pawn = 0;
            root_component = 0;
            camera_manager = 0;
            view_matrix = 0;
            persistent_level = 0;
            actor_array = 0;
            actor_count = 0;
        }
    };
    
    GameAddresses addresses;

    UETerminalReader() : pid_(-1), proc_fd_(-1), libue4_base_(0) {
        // 不再使用 memset，因为 GameAddresses 已经有构造函数初始化
    }

    ~UETerminalReader() { 
        if (proc_fd_ >= 0) close(proc_fd_); 
    }

    bool init(pid_t target_pid) {
        pid_ = target_pid;
        
        if (!find_ue4_module()) {
            std::cerr << "错误: 找不到UE4模块" << std::endl;
            return false;
        }
        
        addresses.libue4 = libue4_base_;
        std::cout << "✅ UE4基址: 0x" << std::hex << libue4_base_ << std::dec << std::endl;
        
        return init_addresses();
    }

    // 核心读取函数
    uintptr_t getPtr64(uintptr_t address) {
        if (address == 0) return 0;
        uintptr_t value = 0;
        read_memory(address, &value, sizeof(value));
        return value;
    }

    uint32_t getPtr32(uintptr_t address) {
        if (address == 0) return 0;
        uint32_t value = 0;
        read_memory(address, &value, sizeof(value));
        return value;
    }

    float getFloat(uintptr_t address) {
        if (address == 0) return 0.0f;
        float value = 0.0f;
        read_memory(address, &value, sizeof(value));
        return value;
    }

    struct Vector3 {
        float x, y, z;
        Vector3() : x(0), y(0), z(0) {}
        Vector3(float x, float y, float z) : x(x), y(y), z(z) {}
        
        std::string toString() const {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(2);
            ss << "(" << x << ", " << y << ", " << z << ")";
            return ss.str();
        }
    };
    
    Vector3 getVector3(uintptr_t address) {
        Vector3 vec;
        if (address == 0) return vec;
        read_memory(address, &vec, sizeof(vec));
        return vec;
    }

    // 更新游戏地址
    bool update_addresses() {
        if (addresses.gworld != 0) {
            addresses.uworld = getPtr64(addresses.gworld);
            
            if (addresses.uworld != 0) {
                addresses.game_instance = getPtr64(addresses.uworld + 0x1A0);
                addresses.local_players = getPtr64(addresses.game_instance + 0x38);
                
                if (addresses.local_players != 0) {
                    addresses.local_player = getPtr64(addresses.local_players);
                    
                    if (addresses.local_player != 0) {
                        addresses.player_controller = getPtr64(addresses.local_player + 0x30);
                        
                        if (addresses.player_controller != 0) {
                            addresses.pawn = getPtr64(addresses.player_controller + 0x3A0);
                            
                            if (addresses.pawn != 0) {
                                addresses.root_component = getPtr64(addresses.pawn + 0x190);
                            }
                        }
                        
                        addresses.camera_manager = getPtr64(addresses.player_controller + 0x340);
                    }
                }
                
                addresses.persistent_level = getPtr64(addresses.uworld + 0x30);
                
                if (addresses.persistent_level != 0) {
                    addresses.actor_array = getPtr64(addresses.persistent_level + 0xA0);
                    addresses.actor_count = getPtr32(addresses.persistent_level + 0xA8);
                }
            }
        }
        
        return true;
    }

    // 终端命令函数
    void show_base_info() {
        std::cout << "\n=== UE4 基础信息 ===" << std::endl;
        std::cout << "进程PID: " << pid_ << std::endl;
        std::cout << "UE4基址: 0x" << std::hex << addresses.libue4 << std::dec << std::endl;
        std::cout << "GWorld: 0x" << std::hex << addresses.gworld << std::dec << std::endl;
        std::cout << "GObjects: 0x" << std::hex << addresses.gobjects << std::dec << std::endl;
        std::cout << "GNames: 0x" << std::hex << addresses.gnames << std::dec << std::endl;
    }

    void show_game_addresses() {
        update_addresses();
        
        std::cout << "\n=== 游戏地址信息 ===" << std::endl;
        std::cout << "UWorld: 0x" << std::hex << addresses.uworld << std::dec << std::endl;
        std::cout << "GameInstance: 0x" << std::hex << addresses.game_instance << std::dec << std::endl;
        std::cout << "LocalPlayer: 0x" << std::hex << addresses.local_player << std::dec << std::endl;
        std::cout << "PlayerController: 0x" << std::hex << addresses.player_controller << std::dec << std::endl;
        std::cout << "Pawn: 0x" << std::hex << addresses.pawn << std::dec << std::endl;
        std::cout << "RootComponent: 0x" << std::hex << addresses.root_component << std::dec << std::endl;
        std::cout << "CameraManager: 0x" << std::hex << addresses.camera_manager << std::dec << std::endl;
        std::cout << "Actor数量: " << addresses.actor_count << std::endl;
    }

    void show_player_info() {
        update_addresses();
        
        std::cout << "\n=== 玩家信息 ===" << std::endl;
        if (addresses.root_component != 0) {
            Vector3 position = getVector3(addresses.root_component + 0x1A0);
            std::cout << "玩家位置: " << position.toString() << std::endl;
        } else {
            std::cout << "无法获取玩家位置" << std::endl;
        }
    }

    void chain_read(const std::string& expression) {
        std::cout << "\n=== 链式读取: " << expression << " ===" << std::endl;
        
        // 解析表达式如: libue4 + 0x12345678 -> 0x20 -> 0x30
        std::vector<std::string> parts;
        std::stringstream ss(expression);
        std::string part;
        
        while (std::getline(ss, part, '-')) {
            if (part.find('>') != std::string::npos) {
                part.erase(0, 1); // 移除 '>'
            }
            parts.push_back(part);
        }
        
        uintptr_t current_addr = 0;
        
        for (size_t i = 0; i < parts.size(); i++) {
            std::string& p = parts[i];
            std::stringstream ps(p);
            std::string token;
            
            // 解析每个部分
            while (std::getline(ps, token, '+')) {
                trim(token);
                if (token == "libue4") {
                    current_addr = addresses.libue4;
                } else if (token.find("0x") == 0) {
                    uintptr_t offset = std::stoul(token, nullptr, 16);
                    if (i == 0 && current_addr == 0) {
                        current_addr = offset;
                    } else {
                        current_addr += offset;
                    }
                } else if (token.find_first_of("0123456789") == 0) {
                    uintptr_t offset = std::stoul(token);
                    current_addr += offset;
                }
            }
            
            // 如果不是最后一部分，读取指针
            if (i < parts.size() - 1) {
                uintptr_t next = getPtr64(current_addr);
                std::cout << "步骤" << i+1 << ": 0x" << std::hex << current_addr 
                         << " -> 0x" << next << std::dec << std::endl;
                current_addr = next;
                if (current_addr == 0) {
                    std::cout << "❌ 链式读取中断，遇到空指针" << std::endl;
                    return;
                }
            }
        }
        
        std::cout << "最终地址: 0x" << std::hex << current_addr << std::dec << std::endl;
        
        // 读取最终地址的值
        uintptr_t final_value = getPtr64(current_addr);
        std::cout << "最终值: 0x" << std::hex << final_value << std::dec << std::endl;
    }

    void custom_read(uintptr_t address) {
        std::cout << "\n=== 自定义读取 ===" << std::endl;
        std::cout << "地址: 0x" << std::hex << address << std::dec << std::endl;
        
        uintptr_t value = getPtr64(address);
        std::cout << "指针值: 0x" << std::hex << value << std::dec << std::endl;
        
        // 尝试读取为浮点数
        float float_value = getFloat(address);
        std::cout << "浮点值: " << float_value << std::endl;
        
        // 尝试读取为向量
        Vector3 vec_value = getVector3(address);
        std::cout << "向量值: " << vec_value.toString() << std::endl;
    }

    void save_address(const std::string& name, uintptr_t address) {
        addresses.custom_addresses[name] = address;
        std::cout << "✅ 保存地址 '" << name << "' = 0x" << std::hex << address << std::dec << std::endl;
    }

    void show_saved_addresses() {
        std::cout << "\n=== 保存的地址 ===" << std::endl;
        if (addresses.custom_addresses.empty()) {
            std::cout << "没有保存的地址" << std::endl;
            return;
        }
        for (const auto& pair : addresses.custom_addresses) {
            std::cout << pair.first << ": 0x" << std::hex << pair.second << std::dec << std::endl;
        }
    }

    void show_help() {
        std::cout << "\n=== 命令帮助 ===" << std::endl;
        std::cout << "base        - 显示基础信息" << std::endl;
        std::cout << "game        - 显示游戏地址" << std::endl;
        std::cout << "player      - 显示玩家信息" << std::endl;
        std::cout << "chain <exp> - 链式读取 (例: chain libue4+0x12345678->0x20->0x30)" << std::endl;
        std::cout << "read <addr> - 读取指定地址" << std::endl;
        std::cout << "save <name> <addr> - 保存地址" << std::endl;
        std::cout << "saved       - 显示保存的地址" << std::endl;
        std::cout << "update      - 更新所有地址" << std::endl;
        std::cout << "help        - 显示帮助" << std::endl;
        std::cout << "exit        - 退出程序" << std::endl;
    }

private:
    // 辅助函数：去除字符串前后空格
    void trim(std::string& str) {
        size_t start = str.find_first_not_of(" \t\n\r");
        size_t end = str.find_last_not_of(" \t\n\r");
        if (start == std::string::npos) {
            str = "";
        } else {
            str = str.substr(start, end - start + 1);
        }
    }

    bool find_ue4_module() {
        char maps_path[64];
        snprintf(maps_path, sizeof(maps_path), "/proc/%d/maps", pid_);
        
        FILE* maps_file = fopen(maps_path, "r");
        if (!maps_file) return false;
        
        char line[512];
        while (fgets(line, sizeof(line), maps_file)) {
            if (strstr(line, "r-xp")) {
                if (strstr(line, "libUE4") || strstr(line, "ebclient") || 
                    strstr(line, ".0") || strstr(line, "game")) {
                    uintptr_t start_addr;
                    sscanf(line, "%lx", &start_addr);
                    libue4_base_ = start_addr;
                    break;
                }
            }
        }
        fclose(maps_file);
        
        return (libue4_base_ != 0);
    }

    bool init_addresses() {
        // 尝试常见偏移
        std::vector<uintptr_t> gworld_offsets = {0x12AFBBF8, 0x12ACB840, 0x123CFA48, 0x5A0, 0x5A8};
        
        for (uintptr_t offset : gworld_offsets) {
            addresses.gworld = getPtr64(libue4_base_ + offset);
            if (is_valid_pointer(addresses.gworld)) {
                std::cout << "✅ 找到GWorld: 偏移 0x" << std::hex << offset << std::dec << std::endl;
                break;
            }
        }
        
        // 如果直接找不到，尝试链式查找
        if (addresses.gworld == 0) {
            uintptr_t intermediate = getPtr64(libue4_base_ + 0x12AFBBF8);
            if (intermediate != 0) {
                addresses.gworld = getPtr64(intermediate + 0x90);
                if (addresses.gworld != 0) {
                    std::cout << "✅ 通过链式找到GWorld" << std::endl;
                }
            }
        }
        
        std::vector<uintptr_t> gobjects_offsets = {0x123CFA48, 0x4A0, 0x4A8};
        for (uintptr_t offset : gobjects_offsets) {
            addresses.gobjects = getPtr64(libue4_base_ + offset);
            if (is_valid_pointer(addresses.gobjects)) break;
        }
        
        std::vector<uintptr_t> gnames_offsets = {0x123CFA50, 0x4B0, 0x4B8};
        for (uintptr_t offset : gnames_offsets) {
            addresses.gnames = getPtr64(libue4_base_ + offset);
            if (is_valid_pointer(addresses.gnames)) break;
        }
        
        update_addresses();
        return (addresses.gworld != 0);
    }

    bool read_memory(uintptr_t remote_addr, void* buffer, size_t size) {
        if (pid_ <= 0 || buffer == nullptr) return false;
        
        struct iovec local_iov = {buffer, size};
        struct iovec remote_iov = {reinterpret_cast<void*>(remote_addr), size};
        
        ssize_t n = syscall(SYS_process_vm_readv, pid_, &local_iov, 1, &remote_iov, 1, 0);
        if (n == static_cast<ssize_t>(size)) return true;
        
        if (!open_proc_mem()) return false;
        ssize_t got = pread(proc_fd_, buffer, size, static_cast<off_t>(remote_addr));
        return (got == static_cast<ssize_t>(size));
    }

    bool is_valid_pointer(uintptr_t ptr) {
        if (ptr == 0) return false;
        if (ptr < 0x10000) return false;
        uint8_t test_byte;
        return read_memory(ptr, &test_byte, sizeof(test_byte));
    }

    bool open_proc_mem() {
        if (proc_fd_ >= 0) return true;
        char path[64];
        snprintf(path, sizeof(path), "/proc/%d/mem", pid_);
        proc_fd_ = open(path, O_RDONLY);
        return (proc_fd_ >= 0);
    }
};

void run_terminal(UETerminalReader& reader) {
    std::cout << "🎮 UE4内存读取器终端模式" << std::endl;
    std::cout << "✨ 永久作者留名须知: 阿念MagicKing" << std::endl;
    std::cout << "🎃 不要试图联系我联系懒得回" << std::endl;
    std::cout << "输入 'help' 查看可用命令" << std::endl;
    
    std::string command;
    while (true) {
        std::cout << "\n>> ";
        std::getline(std::cin, command);
        
        if (command == "exit" || command == "quit") {
            break;
        } else if (command == "base") {
            reader.show_base_info();
        } else if (command == "game") {
            reader.show_game_addresses();
        } else if (command == "player") {
            reader.show_player_info();
        } else if (command.find("chain ") == 0) {
            std::string expression = command.substr(6);
            reader.chain_read(expression);
        } else if (command.find("read ") == 0) {
            std::string addr_str = command.substr(5);
            uintptr_t addr = std::stoul(addr_str, nullptr, 16);
            reader.custom_read(addr);
        } else if (command.find("save ") == 0) {
            std::stringstream ss(command.substr(5));
            std::string name, addr_str;
            ss >> name >> addr_str;
            if (!name.empty() && !addr_str.empty()) {
                uintptr_t addr = std::stoul(addr_str, nullptr, 16);
                reader.save_address(name, addr);
            } else {
                std::cout << "❌ 用法: save <名称> <地址>" << std::endl;
            }
        } else if (command == "saved") {
            reader.show_saved_addresses();
        } else if (command == "update") {
            reader.update_addresses();
            std::cout << "✅ 地址已更新" << std::endl;
        } else if (command == "help") {
            reader.show_help();
        } else {
            std::cout << "❌ 未知命令，输入 'help' 查看帮助" << std::endl;
        }
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "用法: " << argv[0] << " <PID> [命令]" << std::endl;
        std::cout << "示例: " << std::endl;
        std::cout << "  " << argv[0] << " 1234                    # 进入交互模式" << std::endl;
        std::cout << "  " << argv[0] << " 1234 base              # 显示基础信息" << std::endl;
        std::cout << "  " << argv[0] << " 1234 chain libue4+0x12345678->0x20" << std::endl;
        return 1;
    }
    
    pid_t pid = std::stoi(argv[1]);
    
    UETerminalReader reader;
    if (!reader.init(pid)) {
        std::cerr << "初始化失败" << std::endl;
        return 1;
    }
    
    // 如果有额外命令参数，执行单条命令
    if (argc > 2) {
        std::string command;
        for (int i = 2; i < argc; i++) {
            if (i > 2) command += " ";
            command += argv[i];
        }
        
        if (command == "base") {
            reader.show_base_info();
        } else if (command == "game") {
            reader.show_game_addresses();
        } else if (command == "player") {
            reader.show_player_info();
        } else if (command.find("chain ") == 0) {
            std::string expression = command.substr(6);
            reader.chain_read(expression);
        } else if (command.find("read ") == 0) {
            std::string addr_str = command.substr(5);
            uintptr_t addr = std::stoul(addr_str, nullptr, 16);
            reader.custom_read(addr);
        } else {
            std::cout << "未知命令: " << command << std::endl;
        }
    } else {
        // 进入交互模式
        run_terminal(reader);
    }
    
    return 0;
}