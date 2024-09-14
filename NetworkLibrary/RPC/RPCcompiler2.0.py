#RPCcompiler2.0
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
            if 'RegisterClass' in line[0]:
                registered_classes.append(line[-1].replace(' ',""))
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

def write_server_proxy_header(file_name, class_name, functions, basic_type, registered_classes):
    with open(file_name, 'wt') as fout:
        fout.writelines('#pragma once\n')
        fout.writelines('#include "Network/Session.h"\n')
        fout.writelines('#include "Container/MyStlContainer.h"\n')
        for registered_name in registered_classes:
            fout.writelines(f'class {registered_name};\n')
        fout.writelines(f'class {class_name}ServerProxy\n')
        fout.writelines('{\nprivate:\n\tclass IOCPServer* _pServer;\npublic:\n')
        for func_name, param_types, param_names in functions:
            tail = ', '.join(f'{"const " if ptype not in basic_type else ""}{ptype}{"&" if ptype not in basic_type else ""} {pname}' for ptype, pname in zip(param_types, param_names))
            if len(tail) > 0 :     
                tail+=', '
            fout.writelines(f'\tvoid {func_name}(SessionInfo sessionInfo, {tail}bool bDisconnect = false);\n')
            fout.writelines(f'\tvoid {func_name}(const List<SessionInfo>& sessionInfoList, {tail}bool bDisconnect = false);\n\n')
        fout.writelines(f'\t{class_name}ServerProxy(class IOCPServer* pServer)\n\t{{\n\t\t_pServer = pServer;\n\t}}\n}};\n')

def write_server_proxy_cpp(file_name, class_name, functions, basic_type, registered_classes):
    with open(file_name, 'wt') as fout:
        fout.writelines(f'#include "{class_name}ServerProxy.h"\n')
        fout.writelines(f'#include "{class_name}PKT_TYPE.h"\n')
        fout.writelines('#include "Network/IOCPServer.h"\n')
        for registered_name in registered_classes:
            fout.writelines(f'#include "{registered_name}.h"\n')
        for func_name, param_types, param_names in functions:
            tail = ', '.join(f'{"const " if ptype not in basic_type else ""}{ptype}{"&" if ptype not in basic_type else ""} {pname}' for ptype, pname in zip(param_types, param_names))
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
            fout.writelines(f'void {class_name}ServerProxy::{func_name}(const List<SessionInfo>& sessionInfoList, {tail}bool bDisconnect)\n{{\n')
            fout.writelines('\tCSendBuffer* pBuf = CSendBuffer::Alloc();\n')
            fout.writelines('\tpBuf->IncrementRefCnt();\n\ttry\n\t{\n\t\t*pBuf')
            fout.writelines(f' << PKT_TYPE_{func_name}')
            for param_name in param_names:
                fout.writelines(f' << {param_name}')
            fout.writelines(';\n\t}\n\tcatch(int useSize)\n\t{\n\t}\n')
            fout.writelines('\tfor(SessionInfo sessionInfo: sessionInfoList)\n\t{\n')
            fout.writelines('\t\t_pServer->Unicast(sessionInfo, pBuf, bDisconnect);\n\t}\n')
            fout.writelines('\tpBuf->DecrementRefCnt();\n}\n')

def write_client_proxy_header(file_name, class_name, functions, basic_type,registered_classes):
    with open(file_name, 'wt') as fout:
        fout.writelines('#pragma once\n')
        fout.writelines('#include "Network/Session.h"\n')
        fout.writelines('#include "Container/MyStlContainer.h"\n')
        for registered_name in registered_classes:
            fout.writelines(f'class {registered_name};\n')
        fout.writelines(f'class {class_name}ClientProxy\n')
        fout.writelines('{\nprivate:\n\tclass IOCPClient* _pClient;\npublic:\n')
        for func_name, param_types, param_names in functions:
            tail = ', '.join(f'{"const " if ptype not in basic_type else ""}{ptype}{"&" if ptype not in basic_type else ""} {pname}' for ptype, pname in zip(param_types, param_names))
            if len(tail)>0:
                tail+=', '
            fout.writelines(f'\tvoid {func_name}({tail}bool bDisconnect = false);\n')
        fout.writelines(f'\t{class_name}ClientProxy(class IOCPClient* pClient)\n\t{{\n\t\t_pClient = pClient;\n\t}}\n}};\n')

def write_client_proxy_cpp(file_name, class_name, functions, basic_type, registered_classes):
    with open(file_name, 'wt') as fout:
        fout.writelines(f'#include "{class_name}ClientProxy.h"\n')
        fout.writelines(f'#include "{class_name}PKT_TYPE.h"\n')
        fout.writelines('#include "Network/IOCPClient.h"\n')
        for registered_name in registered_classes:
            fout.writelines(f'#include "{registered_name}.h"\n')
        for func_name, param_types, param_names in functions:
            tail = ', '.join(f'{"const " if ptype not in basic_type else ""}{ptype}{"&" if ptype not in basic_type else ""} {pname}' for ptype, pname in zip(param_types, param_names))
            if len(tail)>0:
                tail+=', '
            fout.writelines(f'void {class_name}ClientProxy::{func_name}({tail}bool bDisconnect)\n{{\n')
            fout.writelines('\tCSendBuffer* pBuf = CSendBuffer::Alloc();\n')
            fout.writelines('\tpBuf->IncrementRefCnt();\n\ttry\n\t{\n\t\t*pBuf')
            fout.writelines(f' << PKT_TYPE_{func_name}')
            for param_name in param_names:
                fout.writelines(f' << {param_name}')
            fout.writelines(';\n\t}\n\tcatch(int useSize)\n\t{\n\t}\n')
            fout.writelines('\t_pClient->Unicast(pBuf, bDisconnect);\n')
            fout.writelines('\tpBuf->DecrementRefCnt();\n}\n')

def write_server_stub_header(file_name, class_name, functions, basic_type,registered_classes):
    with open(file_name, 'wt') as fout:
        fout.writelines('#pragma once\n')
        fout.writelines('#include "Network/Session.h"\n')
        fout.writelines('#include "Buffer/CRecvBuffer.h"\n')
        fout.writelines('#include "Container/MyStlContainer.h"\n')
        fout.writelines(f'#include "{class_name}PKT_TYPE.h"\n')
        for registered_name in registered_classes:
            fout.writelines(f'class {registered_name};\n')
        fout.writelines(f'class {class_name}ServerStub\n')
        fout.writelines('{\npublic:\n')
        for func_name, param_types, param_names in functions:
            tail = ', '.join(f'{ptype}{"&" if ptype not in basic_type else ""} {pname}' for ptype, pname in zip(param_types, param_names))
            packet_proc_declaration = f"bool PacketProc{func_name}(SessionInfo sessionInfo, int roomID, CRecvBuffer& buf)"
            proc_declaration = f"virtual void Proc{func_name}(SessionInfo sessionInfo, int roomID"
            if len(tail)>0:
                proc_declaration+=', '
            proc_declaration += tail
            proc_declaration += ')'
            fout.writelines(f'\t{packet_proc_declaration};\n')
            fout.writelines(f'\t{proc_declaration} {{}}\n\n')
        
        fout.writelines('\tbool PacketProc(SessionInfo sessionInfo, int roomID, CRecvBuffer& buf);\n')
        fout.writelines('};\n')

def write_server_stub_cpp(file_name, class_name, functions,registered_classes):
    with open(file_name, 'wt') as fout:
        fout.writelines(f'#include "{class_name}ServerStub.h"\n')
        fout.writelines('#include "DebugTool/Log.h"\n')
        for registered_name in registered_classes:
            fout.writelines(f'#include "{registered_name}.h"\n')
        for func_name, param_types, param_names in functions:
            packet_proc_declaration = f"bool {class_name}ServerStub::PacketProc{func_name}(SessionInfo sessionInfo, int roomID, CRecvBuffer& buf)"
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
            packet_proc_def += f'\tProc{func_name}(sessionInfo, roomID'
            
            for pname in param_names:
                packet_proc_def += f', {pname}'
            
            packet_proc_def += ');\n\treturn true;\n}\n'
            fout.writelines(f'{packet_proc_declaration}\n{packet_proc_def}\n')
        
        fout.writelines(f'bool {class_name}ServerStub::PacketProc(SessionInfo sessionInfo,int roomID, CRecvBuffer& buf)\n')
        fout.writelines('{\n\tshort packetType;\n')
        fout.writelines('\ttry\n\t{\n\t\tbuf >> packetType;\n\t}\n')
        fout.writelines('\tcatch(int remainSize)\n\t{\n\t\treturn false;\n\t}\n')
        fout.writelines('\tswitch(packetType)\n\t{\n')
        
        for func_name, _, _ in functions:
            fout.writelines(f'\tcase PKT_TYPE_{func_name}:\n\t{{\n')
            fout.writelines(f'\t\treturn PacketProc{func_name}(sessionInfo,roomID, buf);\n')
            fout.writelines('\t\tbreak;\n\t}\n')
        fout.writelines('\tdefault:\n\t{\n\t\treturn false;\n\t}\n\t}\n}\n')        

def write_client_stub_header(file_name, class_name, functions, basic_type, registered_classes):
     with open(file_name, 'wt') as fout:
        fout.writelines('#pragma once\n')
        fout.writelines('#include "Network/Session.h"\n')
        fout.writelines('#include "Buffer/CRecvBuffer.h"\n')
        fout.writelines('#include "Container/MyStlContainer.h"\n')
        fout.writelines(f'#include "{class_name}PKT_TYPE.h"\n')
        for registered_name in registered_classes:
            fout.writelines(f'class {registered_name};\n')
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

def write_client_stub_cpp(file_name, class_name, functions, registered_classes):
    with open(file_name, 'wt') as fout:
        fout.writelines(f'#include "{class_name}ClientStub.h"\n')
        fout.writelines('#include "DebugTool/Log.h"\n')
        for registered_name in registered_classes:
            fout.writelines(f'#include "{registered_name}.h"\n')
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
            input_params = ', '.join(param_names)
            packet_proc_def += input_params
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

def write_dummy_client_proxy_header(file_name, class_name, functions, basic_type, registered_classes):
    with open(file_name, 'wt') as fout:
        fout.writelines('#pragma once\n')
        fout.writelines('#include "Network/Session.h"\n')
        fout.writelines('#include "Container/MyStlContainer.h"\n')
        for registered_name in registered_classes:
            fout.writelines(f'class {registered_name};\n')
        fout.writelines(f'class {class_name}DummyProxy\n')
        fout.writelines('{\nprivate:\n\tclass IOCPDummyClient* _pDummyClient;\npublic:\n')
        for func_name, param_types, param_names in functions:
            tail = ', '.join(f'{"const " if ptype not in basic_type else ""}{ptype}{"&" if ptype not in basic_type else ""} {pname}' for ptype, pname in zip(param_types, param_names))
            if len(tail) > 0 :     
                tail+=', '
            fout.writelines(f'\tvoid {func_name}(SessionInfo sessionInfo, {tail} bool bDisconnect = false);\n')
            fout.writelines(f'\tvoid {func_name}(const List<SessionInfo>& sessionInfoList, {tail} bool bDisconnect = false);\n\n')
        fout.writelines(f'\t{class_name}DummyProxy(class IOCPDummyClient* pDummyClient)\n\t{{\n\t\t_pDummyClient = pDummyClient;\n\t}}\n}};\n')

def write_dummy_client_proxy_cpp(file_name, class_name, functions, basic_type, registered_classes):
    with open(file_name, 'wt') as fout:
        fout.writelines(f'#include "{class_name}DummyProxy.h"\n')
        fout.writelines(f'#include "{class_name}PKT_TYPE.h"\n')
        fout.writelines('#include "Network/IOCPDummyClient.h"\n')
        for registered_name in registered_classes:
            fout.writelines(f'#include "{registered_name}.h"\n')
        for func_name, param_types, param_names in functions:
            tail = ', '.join(f'{"const " if ptype not in basic_type else ""}{ptype}{"&" if ptype not in basic_type else ""} {pname}' for ptype, pname in zip(param_types, param_names))
            if len(tail) > 0 : 
                tail += ', '
            fout.writelines(f'void {class_name}DummyProxy::{func_name}(SessionInfo sessionInfo, {tail}bool bDisconnect)\n{{\n')
            fout.writelines('\tCSendBuffer* pBuf = CSendBuffer::Alloc();\n')
            fout.writelines('\tpBuf->IncrementRefCnt();\n\ttry\n\t{\n\t\t*pBuf')
            fout.writelines(f' << PKT_TYPE_{func_name}')
            for param_name in param_names:
                fout.writelines(f' << {param_name}')
            fout.writelines(';\n\t}\n\tcatch(int useSize)\n\t{\n\t}\n')
            fout.writelines('\t_pDummyClient->Unicast(sessionInfo, pBuf, bDisconnect);\n')
            fout.writelines('\tpBuf->DecrementRefCnt();\n}\n')
            fout.writelines(f'void {class_name}DummyProxy::{func_name}(const List<SessionInfo>& sessionInfoList, {tail}bool bDisconnect)\n{{\n')
            fout.writelines('\tCSendBuffer* pBuf = CSendBuffer::Alloc();\n')
            fout.writelines('\tpBuf->IncrementRefCnt();\n\ttry\n\t{\n\t\t*pBuf')
            fout.writelines(f' << PKT_TYPE_{func_name}')
            for param_name in param_names:
                fout.writelines(f' << {param_name}')
            fout.writelines(';\n\t}\n\tcatch(int useSize)\n\t{\n\t}\n')
            fout.writelines('\tfor(SessionInfo sessionInfo: sessionInfoList)\n\t{\n')
            fout.writelines('\t\t_pDummyClient->Unicast(sessionInfo, pBuf, bDisconnect);\n\t}\n')
            fout.writelines('\tpBuf->DecrementRefCnt();\n}\n')

def write_dummy_stub_header(file_name, class_name, functions, basic_type,registered_classes):
    with open(file_name, 'wt') as fout:
        fout.writelines('#pragma once\n')
        fout.writelines('#include "Network/Session.h"\n')
        fout.writelines('#include "Buffer/CRecvBuffer.h"\n')
        fout.writelines('#include "Container/MyStlContainer.h"\n')
        fout.writelines(f'#include "{class_name}PKT_TYPE.h"\n')
        for registered_name in registered_classes:
            fout.writelines(f'class {registered_name};\n')
        fout.writelines(f'class {class_name}DummyStub\n')
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

def write_dummy_stub_cpp(file_name, class_name, functions,registered_classes):
    with open(file_name, 'wt') as fout:
        fout.writelines(f'#include "{class_name}DummyStub.h"\n')
        fout.writelines('#include "DebugTool/Log.h"\n')
        for registered_name in registered_classes:
            fout.writelines(f'#include "{registered_name}.h"\n')
        for func_name, param_types, param_names in functions:
            packet_proc_declaration = f"bool {class_name}DummyStub::PacketProc{func_name}(SessionInfo sessionInfo, CRecvBuffer& buf)"
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
        
        fout.writelines(f'bool {class_name}DummyStub::PacketProc(SessionInfo sessionInfo, CRecvBuffer& buf)\n')
        fout.writelines('{\n\tshort packetType;\n')
        fout.writelines('\ttry\n\t{\n\t\tbuf >> packetType;\n\t}\n')
        fout.writelines('\tcatch(int remainSize)\n\t{\n\t\treturn false;\n\t}\n')
        fout.writelines('\tswitch(packetType)\n\t{\n')
        
        for func_name, _, _ in functions:
            fout.writelines(f'\tcase PKT_TYPE_{func_name}:\n\t{{\n')
            fout.writelines(f'\t\treturn PacketProc{func_name}(sessionInfo, buf);\n')
            fout.writelines('\t\tbreak;\n\t}\n')
        fout.writelines('\tdefault:\n\t{\n\t\treturn false;\n\t}\n\t}\n}\n')        

def main():
    if len(sys.argv) != 3:
        print("Insufficient arguments")
        sys.exit()
    file_path = sys.argv[1]
    class_name = sys.argv[2]
    registered_classes = []
    with open(file_path, "rt", encoding='utf-8') as fin:
        name_and_typenum = read_initial_info(fin)
        skip_to_brace(fin)
        basic_type = ["int", "unsigned int", "short", "unsigned short", "long", "unsigned long", "long long",
                      "unsigned long long", "bool", "char", "unsigned char", "float", "double", "ULONG64", "LONG64",
                      "BYTE", "INT64", "WORD", "USHORT"]
        functions, designated_pkt_type = parse_functions(fin, registered_classes)
    print(registered_classes)
    write_pkt_type_header(f"{class_name}PKT_TYPE.h", name_and_typenum, functions, designated_pkt_type)
    
    write_server_proxy_header(f"{class_name}ServerProxy.h", class_name, functions, basic_type,registered_classes)
    write_server_proxy_cpp(f"{class_name}ServerProxy.cpp", class_name, functions, basic_type,registered_classes)
    
    write_client_proxy_header(f"{class_name}ClientProxy.h", class_name, functions, basic_type,registered_classes)
    write_client_proxy_cpp(f"{class_name}ClientProxy.cpp", class_name, functions, basic_type,registered_classes)
    
    write_server_stub_header(f"{class_name}ServerStub.h", class_name, functions, basic_type,registered_classes)
    write_server_stub_cpp(f"{class_name}ServerStub.cpp", class_name, functions,registered_classes)
    
    write_client_stub_header(f"{class_name}ClientStub.h", class_name, functions, basic_type,registered_classes)
    write_client_stub_cpp(f"{class_name}ClientStub.cpp", class_name, functions,registered_classes)

    write_dummy_client_proxy_header(f"{class_name}DummyProxy.h", class_name, functions, basic_type,registered_classes)
    write_dummy_client_proxy_cpp(f"{class_name}DummyProxy.cpp", class_name, functions, basic_type,registered_classes)

    write_dummy_stub_header(f"{class_name}DummyStub.h", class_name, functions, basic_type,registered_classes)
    write_dummy_stub_cpp(f"{class_name}DummyStub.cpp", class_name, functions,registered_classes)


if __name__ == '__main__':
    main()
