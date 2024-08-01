import sys

def read_initial_info(file):
    name_and_typenum = []
    line = file.readline().strip()
    if line:
        name_and_typenum.extend(word for word in line.split() if word != ' ')
    return name_and_typenum

def skip_to_brace(file):
    while True:
        line = file.readline().strip()
        if line == '{':
            break

def parse_functions(file, registered_classes):
    functions = []
    designated_pkt_type = {}
    while True:
        one_func_parameters_type = []
        one_func_parameters_name = []
        line = file.readline()
        if line == '' :
            break
        line = line.replace('\n','')
        if line == '}':
            break
        if '//' in line:
            line = line.split('//')[0].strip()
        if not line:
            continue
        pkt_num = -1
        if ':' in line:
            line = line.replace('\t', '').split(':')
            if line[0] == 'RegisterClass':
                registered_classes.append(line[1])
                continue
            pkt_num = line[-1].strip()
            line = line[0]

        line = line.replace(')', '').replace('\t', '').strip()
        if not line:
            continue
        
        func_name, parameters = line.split('(')
        if pkt_num != -1:
            designated_pkt_type[func_name] = pkt_num
        parameters = [param.strip() for param in parameters.split(',')]
        
        for parameter in parameters:
            parts = [p for p in parameter.split(' ') if p]
            if len(parts) == 0:
                break
            if '*' in parts[-1]:
                parts[-1] = parts[-1].replace('*', '')
                parts[-2] += '*'
            one_func_parameters_name.append(parts[-1])
            type_ = ' '.join(parts[:-1])
            if "Array" in type_:
                array_size = type_.split('[')[1]
                type_ = type_.split('[')[0]
                type_=type_.replace('>','') 
                type_ += ',' + array_size.replace(']', '') + '>'
            one_func_parameters_type.append(type_)
        
        functions.append((func_name, one_func_parameters_type, one_func_parameters_name))
    return functions, designated_pkt_type

def write_pkt_type_header(file_name, name_and_typenum, functions, designated_pkt_type):
    with open(file_name, 'wt') as fout:
        fout.writelines('#pragma once\n')
        fout.writelines('typedef const short PKT_TYPE;\n')
        name_and_typenum[1] = int(name_and_typenum[1])
        for func_name, _, _ in functions:
            pkt_type_def = f'PKT_TYPE PKT_TYPE_{func_name} = '
            if func_name in designated_pkt_type:
                pkt_type_def += str(designated_pkt_type[func_name])
                name_and_typenum[1] = int(designated_pkt_type[func_name]) + 1
            else:
                pkt_type_def += str(name_and_typenum[1])
                name_and_typenum[1] += 1
            pkt_type_def += ';\n\n'
            fout.writelines(pkt_type_def)

def write_server_proxy_header(file_name, class_name, functions, basic_type):
    with open(file_name, 'wt') as fout:
        fout.writelines('#pragma once\n')
        fout.writelines('#include "Session.h"\n')
        fout.writelines('#include "IOCPServer.h"\n')
        fout.writelines('#include "MyStlContainer.h"\n')
        fout.writelines(f'class {class_name}ServerProxy\n')
        fout.writelines('{\nprivate:\n\tIOCPServer* _pServer;\npublic:\n')
        for func_name, param_types, param_names in functions:
            tail = ', '.join(f'{ptype}{"&" if ptype not in basic_type else ""} {pname}' for ptype, pname in zip(param_types, param_names))
            if len(tail) > 0 :     
                fout.writelines(f'\tvoid {func_name}(SessionInfo sessionInfo, {tail}, bool bDisconnect = false);\n')
                fout.writelines(f'\tvoid {func_name}(List<SessionInfo>& sessionInfoList, {tail}, bool bDisconnect = false);\n\n')
            else:
                fout.writelines(f'\tvoid {func_name}(SessionInfo sessionInfo, bool bDisconnect = false);\n')
                fout.writelines(f'\tvoid {func_name}(List<SessionInfo>& sessionInfoList, bool bDisconnect = false);\n\n')
        fout.writelines(f'\t{class_name}ServerProxy(IOCPServer* pServer)\n\t{{\n\t\t_pServer = pServer;\n\t}}\n}};\n')

def write_server_proxy_cpp(file_name, class_name, functions, basic_type):
    with open(file_name, 'wt') as fout:
        fout.writelines(f'#include "{class_name}ServerProxy.h"\n')
        fout.writelines(f'#include "{class_name}PKT_TYPE.h"\n')
        for func_name, param_types, param_names in functions:
            tail = ', '.join(f'{ptype}{"&" if ptype not in basic_type else ""} {pname}' for ptype, pname in zip(param_types, param_names))
            if len(tail) > 0 : 
                tail += ', '
            fout.writelines(f'void {class_name}ServerProxy::{func_name}(SessionInfo sessionInfo, {tail}bool bDisconnect)\n{{\n')
            fout.writelines('\tCSendBuffer* pBuf = CSendBuffer::Alloc();\n')
            fout.writelines('\tpBuf->IncrementRefCnt();\n\ttry\n\t{\n\t\t*pBuf')
            fout.writelines(f' << PKT_TYPE_{func_name}')
            for param_name in param_names:
                fout.writelines(f' << {param_name}')
            fout.writelines(';\n\t}\n\tcatch(int useSize)\n\t{\n\t}\n')
            fout.writelines('\t_pServer->Unicast(sessionInfo, pBuf, bDisconnect);\n')
            fout.writelines('\tpBuf->DecrementRefCnt();\n}\n')
            fout.writelines(f'void {class_name}ServerProxy::{func_name}(List<SessionInfo>& sessionInfoList, {tail}bool bDisconnect)\n{{\n')
            fout.writelines('\tCSendBuffer* pBuf = CSendBuffer::Alloc();\n')
            fout.writelines('\tpBuf->IncrementRefCnt();\n\ttry\n\t{\n\t\t*pBuf')
            fout.writelines(f' << PKT_TYPE_{func_name}')
            for param_name in param_names:
                fout.writelines(f' << {param_name}')
            fout.writelines(';\n\t}\n\tcatch(int useSize)\n\t{\n\t}\n')
            fout.writelines('\tfor(SessionInfo sessionInfo: sessionInfoList)\n\t{\n')
            fout.writelines('\t\t_pServer->Unicast(sessionInfo, pBuf, bDisconnect);\n\t}\n')
            fout.writelines('\tpBuf->DecrementRefCnt();\n}\n')

def write_client_proxy_header(file_name, class_name, functions, basic_type):
    with open(file_name, 'wt') as fout:
        fout.writelines('#pragma once\n')
        fout.writelines('#include "Session.h"\n')
        fout.writelines('#include "IOCPClient.h"\n')
        fout.writelines('#include "MyStlContainer.h"\n')
        fout.writelines(f'class {class_name}ClientProxy\n')
        fout.writelines('{\nprivate:\n\tIOCPClient* _pClient;\npublic:\n')
        for func_name, param_types, param_names in functions:
            tail = ', '.join(f'{ptype}{"&" if ptype not in basic_type else ""} {pname}' for ptype, pname in zip(param_types, param_names))
            fout.writelines(f'\tvoid {func_name}({tail}, bool bDisconnect = false);\n')
        fout.writelines(f'\t{class_name}ClientProxy(IOCPClient* pClient)\n\t{{\n\t\t_pClient = pClient;\n\t}}\n}};\n')

def write_client_proxy_cpp(file_name, class_name, functions, basic_type):
    with open(file_name, 'wt') as fout:
        fout.writelines(f'#include "{class_name}ClientProxy.h"\n')
        fout.writelines(f'#include "{class_name}PKT_TYPE.h"\n')
        for func_name, param_types, param_names in functions:
            tail = ', '.join(f'{ptype}{"&" if ptype not in basic_type else ""} {pname}' for ptype, pname in zip(param_types, param_names))
            fout.writelines(f'void {class_name}ClientProxy::{func_name}({tail}, bool bDisconnect)\n{{\n')
            fout.writelines('\tCSendBuffer* pBuf = CSendBuffer::Alloc();\n')
            fout.writelines('\tpBuf->IncrementRefCnt();\n\ttry\n\t{\n\t\t*pBuf')
            fout.writelines(f' << PKT_TYPE_{func_name}')
            for param_name in param_names:
                fout.writelines(f' << {param_name}')
            fout.writelines(';\n\t}\n\tcatch(int useSize)\n\t{\n\t}\n')
            fout.writelines('\t_pClient->Unicast(pBuf, bDisconnect);\n')
            fout.writelines('\tpBuf->DecrementRefCnt();\n}\n')

def write_server_stub_header(file_name, class_name, functions, basic_type):
    with open(file_name, 'wt') as fout:
        fout.writelines('#pragma once\n')
        fout.writelines('#include "Session.h"\n')
        fout.writelines('#include "CRecvBuffer.h"\n')
        fout.writelines('#include "MyStlContainer.h"\n')
        fout.writelines(f'#include "{class_name}PKT_TYPE.h"\n')
        
        fout.writelines(f'class {class_name}ServerStub\n')
        fout.writelines('{\npublic:\n')
        
        for func_name, param_types, param_names in functions:
            tail = ', '.join(f'{ptype}{"&" if ptype not in basic_type else ""} {pname}' for ptype, pname in zip(param_types, param_names))
            packet_proc_declaration = f"bool PacketProc{func_name}(SessionInfo sessionInfo, CRecvBuffer& buf)"
            proc_declaration = f"virtual void Proc{func_name}(SessionInfo sessionInfo"
            if len(tail)>0:
                proc_declaration+=', '
            proc_declaration += tail
            proc_declaration += ')'
            fout.writelines(f'\t{packet_proc_declaration};\n')
            fout.writelines(f'\t{proc_declaration} {{}}\n\n')
        
        fout.writelines('\tbool PacketProc(SessionInfo sessionInfo, CRecvBuffer& buf);\n')
        fout.writelines('};\n')

def write_server_stub_cpp(file_name, class_name, functions):
    with open(file_name, 'wt') as fout:
        fout.writelines(f'#include "{class_name}ServerStub.h"\n')
        fout.writelines('#include "IOCPServer.h"\n')
        fout.writelines('#include "Log.h"\n')
        fout.writelines('using namespace std;\n')
        
        for func_name, param_types, param_names in functions:
            packet_proc_declaration = f"bool {class_name}ServerStub::PacketProc{func_name}(SessionInfo sessionInfo, CRecvBuffer& buf)"
            packet_proc_def = '{\n'
            for ptype, pname in zip(param_types, param_names):
                packet_proc_def +='\t'+ptype
                packet_proc_def +=' '+ pname + ';\n'
            
            packet_proc_def += '\ttry\n\t{\n\t\tbuf'
            
            for pname in param_names:
                packet_proc_def +=' >> '+ pname
            packet_proc_def += ';\n\t}\n\tcatch(int useSize)\n\t{\n'
            packet_proc_def += f'\t\tLog::LogOnFile(Log::DEBUG_LEVEL, "PacketProc{func_name} error\\n");\n'
            packet_proc_def += '\t\treturn false;\n\t}\n'
            packet_proc_def += f'\tProc{func_name}(sessionInfo'
            
            for pname in param_names:
                packet_proc_def += f', {pname}'
            
            packet_proc_def += ');\n\treturn true;\n}\n'
            fout.writelines(f'{packet_proc_declaration}\n{packet_proc_def}\n')
        
        fout.writelines(f'bool {class_name}ServerStub::PacketProc(SessionInfo sessionInfo, CRecvBuffer& buf)\n')
        fout.writelines('{\n\tshort packetType;\n')
        fout.writelines('\ttry\n\t{\n\t\tbuf >> packetType;\n\t}\n')
        fout.writelines('\tcatch(int remainSize)\n\t{\n\t\treturn false;\n\t}\n')
        fout.writelines('\tswitch(packetType)\n\t{\n')
        
        for func_name, _, _ in functions:
            fout.writelines(f'\tcase PKT_TYPE_{func_name}:\n\t{{\n')
            fout.writelines(f'\t\treturn PacketProc{func_name}(sessionInfo, buf);\n')
            fout.writelines('\t\tbreak;\n\t}\n')
        fout.writelines('\tdefault:\n\t{\n\t\treturn false;\n\t}\n\t}\n}\n')        

def write_client_stub_header(file_name, class_name, functions, basic_type):
     with open(file_name, 'wt') as fout:
        fout.writelines('#pragma once\n')
        fout.writelines('#include "Session.h"\n')
        fout.writelines('#include "CRecvBuffer.h"\n')
        fout.writelines('#include "MyStlContainer.h"\n')
        fout.writelines(f'#include "{class_name}PKT_TYPE.h"\n')
        
        fout.writelines(f'class {class_name}ClientStub\n')
        fout.writelines('{\npublic:\n')
        
        for func_name, param_types, param_names in functions:
            tail = ', '.join(f'{ptype}{"&" if ptype not in basic_type else ""} {pname}' for ptype, pname in zip(param_types, param_names))
            packet_proc_declaration = f"bool PacketProc{func_name}(CRecvBuffer& buf)"
            proc_declaration = f"virtual void Proc{func_name}("
            proc_declaration += tail
            proc_declaration += ')'
            fout.writelines(f'\t{packet_proc_declaration};\n')
            fout.writelines(f'\t{proc_declaration} {{}}\n\n')
        
        fout.writelines('\tbool PacketProc(CRecvBuffer& buf);\n')
        fout.writelines('};\n')

def write_client_stub_cpp(file_name, class_name, functions):
    with open(file_name, 'wt') as fout:
        fout.writelines(f'#include "{class_name}ClientStub.h"\n')
        fout.writelines('#include "IOCPServer.h"\n')
        fout.writelines('#include "Log.h"\n')
        fout.writelines('using namespace std;\n')
        
        for func_name, param_types, param_names in functions:
            packet_proc_declaration = f"bool {class_name}ClientStub::PacketProc{func_name}(CRecvBuffer& buf)"
            packet_proc_def = '{\n'
            for ptype, pname in zip(param_types, param_names):
                packet_proc_def +='\t'+ptype
                packet_proc_def +=' '+ pname + ';\n'
            
            packet_proc_def += '\ttry\n\t{\n\t\tbuf'
            
            for pname in param_names:
                packet_proc_def +=' >> '+ pname
            packet_proc_def += ';\n\t}\n\tcatch(int useSize)\n\t{\n'
            packet_proc_def += f'\t\tLog::LogOnFile(Log::DEBUG_LEVEL, "PacketProc{func_name} error\\n");\n'
            packet_proc_def += '\t\treturn false;\n\t}\n'
            packet_proc_def += f'\tProc{func_name}('
            
            for pname in param_names:
                packet_proc_def += f', {pname}'
            
            packet_proc_def += ');\n\treturn true;\n}\n'
            fout.writelines(f'{packet_proc_declaration}\n{packet_proc_def}\n')
        
        fout.writelines(f'bool {class_name}ClientStub::PacketProc(CRecvBuffer& buf)\n')
        fout.writelines('{\n\tshort packetType;\n')
        fout.writelines('\ttry\n\t{\n\t\tbuf >> packetType;\n\t}\n')
        fout.writelines('\tcatch(int remainSize)\n\t{\n\t\treturn false;\n\t}\n')
        fout.writelines('\tswitch(packetType)\n\t{\n')
        
        for func_name, _, _ in functions:
            fout.writelines(f'\tcase PKT_TYPE_{func_name}:\n\t{{\n')
            fout.writelines(f'\t\treturn PacketProc{func_name}(buf);\n')
            fout.writelines('\t\tbreak;\n\t}\n')
        fout.writelines('\tdefault:\n\t{\n\t\treturn false;\n\t}\n\t}\n}\n')

def write_dummy_client_proxy_header(file_name, class_name, functions, basic_type):
    with open(file_name, 'wt') as fout:
        fout.writelines('#pragma once\n')
        fout.writelines('#include "Session.h"\n')
        fout.writelines('#include "IOCPDummyClient.h"\n')
        fout.writelines('#include "MyStlContainer.h"\n')
        fout.writelines(f'class {class_name}DummyClientProxy\n')
        fout.writelines('{\nprivate:\n\tIOCPDummyClient* _pClient;\npublic:\n')
        for func_name, param_types, param_names in functions:
            tail = ', '.join(f'{ptype}{"&" if ptype not in basic_type else ""} {pname}' for ptype, pname in zip(param_types, param_names))
            fout.writelines(f'\tvoid {func_name}({tail}, bool bDisconnect = false);\n')
        fout.writelines(f'\t{class_name}DummyClientProxy(IOCPDummyClient* pClient)\n\t{{\n\t\t_pDummyClient = pDummyClient;\n\t}}\n}};\n')

def write_dummy_client_proxy_cpp(file_name, class_name, functions, basic_type):
    with open(file_name, 'wt') as fout:
        fout.writelines(f'#include "{class_name}DummyClientProxy.h"\n')
        fout.writelines(f'#include "{class_name}PKT_TYPE.h"\n')
        for func_name, param_types, param_names in functions:
            tail = ', '.join(f'{ptype}{"&" if ptype not in basic_type else ""} {pname}' for ptype, pname in zip(param_types, param_names))
            fout.writelines(f'void {class_name}DummyClientProxy::{func_name}({tail}, bool bDisconnect)\n{{\n')
            fout.writelines('\tCSendBuffer* pBuf = CSendBuffer::Alloc();\n')
            fout.writelines('\tpBuf->IncrementRefCnt();\n\ttry\n\t{\n\t\t*pBuf')
            fout.writelines(f' << PKT_TYPE_{func_name}')
            for param_name in param_names:
                fout.writelines(f' << {param_name}')
            fout.writelines(';\n\t}\n\tcatch(int useSize)\n\t{\n\t}\n')
            fout.writelines('\t_pClient->Unicast(pBuf, bDisconnect);\n')
            fout.writelines('\tpBuf->DecrementRefCnt();\n}\n')

def main():
    if len(sys.argv) != 3:
        print("Insufficient arguments")
        sys.exit()
    file_path = sys.argv[1]
    class_name = sys.argv[2]

    with open(file_path, "rt", encoding='utf-8') as fin:
        name_and_typenum = read_initial_info(fin)
        skip_to_brace(fin)
        basic_type = ["int", "unsigned int", "short", "unsigned short", "long", "unsigned long", "long long",
                      "unsigned long long", "bool", "char", "unsigned char", "float", "double", "ULONG64", "LONG64",
                      "BYTE", "INT64", "WORD", "USHORT"]
        registered_classes = []
        functions, designated_pkt_type = parse_functions(fin, registered_classes)
    write_pkt_type_header(f"{class_name}PKT_TYPE.h", name_and_typenum, functions, designated_pkt_type)
    
    write_server_proxy_header(f"{class_name}ServerProxy.h", class_name, functions, basic_type)
    write_server_proxy_cpp(f"{class_name}ServerProxy.cpp", class_name, functions, basic_type)
    
    write_client_proxy_header(f"{class_name}ClientProxy.h", class_name, functions, basic_type)
    write_client_proxy_cpp(f"{class_name}ClientProxy.cpp", class_name, functions, basic_type)
    
    write_server_stub_header(f"{class_name}ServerStub.h", class_name, functions, basic_type)
    write_server_stub_cpp(f"{class_name}ServerStub.cpp", class_name, functions)
    
    write_client_stub_header(f"{class_name}ClientStub.h", class_name, functions, basic_type)
    write_client_stub_cpp(f"{class_name}ClientStub.cpp", class_name, functions)

    write_dummy_client_proxy_header(f"{class_name}DummyClientProxy.h", class_name, functions, basic_type)
    write_dummy_client_proxy_cpp(f"{class_name}DummyClientProxy.cpp", class_name, functions, basic_type)


if __name__ == '__main__':
    main()
