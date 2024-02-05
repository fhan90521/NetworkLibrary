#-*- encoding: utf-8 -*-
import sys
if len(sys.argv) != 3:
    print("Insufficient arguments")
    sys.exit()
file_path = sys.argv[1]
class_name = sys.argv[2]
fin=open(file_path,"rt",encoding='utf-8')
name_and_typenum=[]
every_func_name=[]
every_func_parameters_type=[]
every_func_parameters_name=[]
designated_pkt_type=dict()
registered_classes=[]
while True:
    line = fin.readline()
    if line == '' :
        break
    line = line.replace('\n','')
    if line != '':
        for word in line.split(' ') :
            if word != ' ':
                name_and_typenum.append(word)
        break
while True:
    line = fin.readline()
    if line == '' :
        break
    line = line.replace('\n','')
    if line == '{':
        break

#함수 한 줄씩 읽기
while True:
    one_func_parameters_type=[]
    one_func_parameters_name=[]
    line = fin.readline()
    if line == '' :
        break
    line = line.replace('\n','')
    if line == '}':
        break
    if '//' in line:
        line_split = line.split('//')
        line = line_split[0]
    if line =='':
        continue
    
    pkt_num=-1
    if ':' in line:
        line = line.replace('\t','')
        line = line.replace(' ','')
        line =line.split(':')
        if line[0]=='RegisterClass':
            registered_classes.append(line[1])
            continue
        pkt_num=line[-1]
        line = line[0]
    line = line.replace(')','')
    line = line.replace('\t','')
    if line =='\n':
        continue
    line = line.split('(')
    if len(line)<2:
        continue
    func_name= line[0]
    if pkt_num!=-1:
        designated_pkt_type[func_name]=pkt_num
    every_func_name.append(func_name)
  
    parameters = line[1].split(',')
    for parameter in parameters:
        parameter = parameter.split(' ')
        temp=[]
        for i in range(0,len(parameter)):
            if parameter[i] != '':
                temp.append(parameter[i])
        parameter=temp
        if len(parameter)>0:   
            if '*' in parameter[-1]:
                parameter[-1]=parameter[-1].replace('*','')
                parameter[-2]+='*'
            one_func_parameters_name.append(parameter[-1])
        type=''
        for i in range(0,len(parameter)-1):
            type+= parameter[i]
            if(i != len(parameter)-2):
                type+= ' '
        if "Array" in type:
            array_size = type.split('[')[1]
            type=type.split('[')[0]
            array_size = array_size.replace(']','')
            type=type.replace('>', ","+array_size+">")
        one_func_parameters_type.append(type)
    every_func_parameters_type.append(one_func_parameters_type)
    every_func_parameters_name.append(one_func_parameters_name)
fin.close()
#PKT_TYPE.h 작성
name_and_typenum[1]=int(name_and_typenum[1])
fout= open(class_name+"PKT_TYPE.h",'wt')
fout.writelines('#pragma once'+'\n')
fout.writelines('typedef const short PKT_TYPE;'+'\n')
#fout.writelines('namespace ' +name_and_typenum[0]+'\n')
#fout.writelines('{\n')
for i in range(0,len(every_func_name)):
    pkt_type_def='PKT_TYPE PKT_TYPE_'+every_func_name[i]+' = '
    if every_func_name[i] in designated_pkt_type:
        pkt_type_def += str(designated_pkt_type[every_func_name[i]])
    else:
        pkt_type_def += str(name_and_typenum[1])
        name_and_typenum[1]+=1
    pkt_type_def+=';\n\n'
    fout.writelines(pkt_type_def)    
#fout.writelines('}\n')
fout.close()
#Proxy.h 작성
declaration_tails=[]
fout= open(class_name+"Proxy.h",'wt')
fout.writelines('#pragma once'+'\n')
fout.writelines('#include "Session.h"\n')
fout.writelines('#include ' +'"IOCPServer.h"'+'\n')
fout.writelines('#include ' +'"MyStlContainer.h"'+'\n')
for registered_class in registered_classes:
    fout.writelines('#include "'+registered_class+'.h"\n')
fout.writelines('using namespace std;\n')
#fout.writelines('namespace ' +name_and_typenum[0]+'\n')
#fout.writelines('{\n')
fout.writelines('class '+class_name+'Proxy\n')
fout.writelines('{\n')
fout.writelines('private:\n')
fout.writelines('\tIOCPServer* _pServer;\n')
fout.writelines('public:\n')
for i in range(0,len(every_func_name)):
    func_name=every_func_name[i]
    parameters_name=every_func_parameters_name[i]
    parameters_type=every_func_parameters_type[i]
    tail=''
    for j in range(0,len(parameters_name)):
        tail+=', '
        tail+=parameters_type[j]
        if "Vector" in parameters_type[j]:
            print('a')    
        if parameters_type[j] in registered_classes or "Array" in parameters_type[j] or "Vector" in parameters_type[j]:
            tail+='&'        
        tail+=' '
        tail+=parameters_name[j]
    tail+=' )'
    declaration_tails.append(tail)
    fout.writelines('\tvoid '+func_name+'(SessionInfo sessionInfo'+tail+';\n')
    fout.writelines('\tvoid '+func_name+'(List<SessionInfo>& sessionInfoList'+tail+';\n\n')
fout.writelines('\t'+class_name+'Proxy(IOCPServer* pServer)\n')
fout.writelines('\t{\n')
fout.writelines('\t\t_pServer=pServer;\n')
fout.writelines('\t}\n')
fout.writelines('};\n')
#fout.writelines('}\n')
fout.close()



#Proxy.cpp 작성
fout= open(class_name+"Proxy.cpp",'wt')
fout.writelines("#include " +' "'+class_name+'Proxy.h"\n')
fout.writelines('#include' + ' "'+ class_name+'PKT_TYPE.h"\n')
#fout.writelines('namespace ' +name_and_typenum[0]+'\n')
#fout.writelines('{\n')
for i in range(0,len(declaration_tails)):
    func_def='{\n'
    func_def+='\tCSendBuffer* pBuf = CSendBuffer::Alloc();\n'
    func_def+='\tpBuf->IncrementRefCnt();\n'
    func_def+='\ttry\n'
    func_def+= '\t{\n'
    func_def+='\t\t*pBuf'
    func_def+=" << "+'PKT_TYPE_'+every_func_name[i]
    for j in range(0,len(every_func_parameters_name[i])):
        func_def+=' << '
        func_def+=every_func_parameters_name[i][j]
    func_def+=';\n'
    func_def+='\t}\n'
    func_def+='\tcatch(int useSize)\n'
    func_def+='\t{\n'
    func_def+='\t}\n'
    func_def+='\tif(_pServer->_bWan)\n'
    func_def+='\t{\n'
    func_def+='\t\tpBuf->SetWanHeader();\n'
    func_def+='\t\tpBuf->Encode();\n'
    func_def+='\t}\n'
    func_def+='\telse\n'
    func_def+='\t{\n'
    func_def+='\t\tpBuf->SetLanHeader();\n'
    func_def+='\t}\n'
    fout.writelines('void '+class_name+'Proxy::'+every_func_name[i]+'(SessionInfo sessionInfo'+declaration_tails[i]+'\n')
    fout.writelines(func_def)
    fout.writelines('\t_pServer->Unicast(sessionInfo, pBuf);\n')
    fout.writelines('\tpBuf->DecrementRefCnt();\n}\n')
    
    fout.writelines('void '+class_name+'Proxy::'+every_func_name[i]+'(List<SessionInfo>& sessionInfoList'+declaration_tails[i]+'\n')
    fout.writelines(func_def)
    fout.writelines('\tfor(SessionInfo sessionInfo: sessionInfoList)\n')
    fout.writelines('\t{\n')
    fout.writelines('\t\t_pServer->Unicast(sessionInfo, pBuf);\n')
    fout.writelines('\t}\n')
    fout.writelines('\tpBuf->DecrementRefCnt();\n')
    fout.writelines('}\n')
#fout.writelines('}\n')
fout.close()



for i in range(0,len(every_func_name)):
    func_name=every_func_name[i]
    parameters_name=every_func_parameters_name[i]
    parameters_type=every_func_parameters_type[i]
    for j in range(0,len(parameters_name)):
        if parameters_type[j] in registered_classes:
            parameters_name[j]=parameters_name[j]+"Ptr"
#Stub.h 작성
fout= open(class_name+"Stub.h",'wt')
fout.writelines('#pragma once'+'\n')
fout.writelines('#include "Session.h"\n')
fout.writelines('#include "CRecvBuffer.h"\n')
fout.writelines('#include "MakeUnique.h"\n')
fout.writelines('#include ' +'"MyStlContainer.h"'+'\n')
fout.writelines('#include' + ' "'+ class_name+'PKT_TYPE.h"\n')
for registered_class in registered_classes:
    fout.writelines('#include "'+registered_class+'.h"\n')
#fout.writelines('namespace ' +name_and_typenum[0]+'\n')
#fout.writelines('{\n')
fout.writelines('class '+class_name+'Stub\n')
fout.writelines('{\n')
#fout.writelines('protected:\n')
#fout.writelines('\tProxy* _pProxy=nullptr;\n')
fout.writelines('public:\n')
for i in range(0,len(every_func_name)):
    func_name=every_func_name[i]
    parameters_name=every_func_parameters_name[i]
    parameters_type=every_func_parameters_type[i]
    packet_proc_declaration= "bool PacketProc"+func_name +'( SessionInfo sessionInfo, CRecvBuffer& buf)'
    proc_declaration="virtual void Proc"+func_name+'( SessionInfo sessionInfo'
    for j in range(0,len(parameters_name)):
        proc_declaration+=', '
        if parameters_type[j] in registered_classes:
            proc_declaration+="UniquePtr<"+parameters_type[j]+'>& '
        else:
            proc_declaration+=parameters_type[j]
            if "Array" in parameters_type[j] or "Vector" in parameters_type[j]:
                proc_declaration+="&"    
        proc_declaration+=' '
        proc_declaration+=parameters_name[j]
    proc_declaration+=' )'
    fout.writelines('\t'+packet_proc_declaration+';\n')
    fout.writelines('\t'+proc_declaration+'{}\n\n')
fout.writelines('\tbool PacketProc(SessionInfo sessionInfo, CRecvBuffer& buf);\n')
#fout.writelines('\tvoid AttachProxy(Proxy* pProxy);\n')
fout.writelines('};\n')
#fout.writelines('}\n')
fout.close()

#Stub.cpp 작성
fout= open(class_name+"Stub.cpp",'wt')
fout.writelines("#include " +' "'+class_name+'Stub.h"\n ')
fout.writelines('#include "IOCPServer.h"\n')
fout.writelines('#include <iostream>\n')
fout.writelines('using namespace std;\n')
#fout.writelines('namespace ' +name_and_typenum[0]+'\n')
#fout.writelines('{\n')
for i in range(0,len(every_func_name)):
    func_name=every_func_name[i]
    parameters_name=every_func_parameters_name[i]
    parameters_type=every_func_parameters_type[i]
    packet_proc_declaration= "bool "+class_name+"Stub::PacketProc"+ func_name +'(SessionInfo sessionInfo, CRecvBuffer& buf)'
    packet_proc_def='{\n'
    for j in range(0,len(parameters_name)):
        if parameters_type[j] in registered_classes:
            print(parameters_type[j])
            packet_proc_def+='\t'+"UniquePtr<"+parameters_type[j]+'> '+parameters_name[j]+"= MakeUnique<"+parameters_type[j]+">();\n"
        else:
            packet_proc_def+='\t'+parameters_type[j]+' '+parameters_name[j]+';\n'
    
    packet_proc_def+='\ttry\n'
    packet_proc_def+='\t{\n'

    packet_proc_def+='\t\tbuf'  
    for j in range(0,len(parameters_name)):
        packet_proc_def+=' >> '+parameters_name[j]
        if parameters_type[j] in registered_classes:
            packet_proc_def+=".get()"        
    packet_proc_def+=';\n'
    packet_proc_def+='\t}\n'
    packet_proc_def+='\tcatch(int useSize)\n'
    packet_proc_def+='\t{\n'
    packet_proc_def+='\t\t cout<<" PacketProc'+func_name+' error"<<endl;\n'
    packet_proc_def+='\t\t return false;\n'
    packet_proc_def+='\t}\n'
    packet_proc_def+='\tProc'+func_name+'( sessionInfo '
    for j in range(0,len(parameters_name)):
        packet_proc_def+=', '
        if parameters_type[j] in registered_classes:
            #packet_proc_def+="move("+ parameters_name[j]+")"
            packet_proc_def+=parameters_name[j]
        else:
            packet_proc_def+=parameters_name[j]
    packet_proc_def+=');\n'
    packet_proc_def+='\treturn true;\n'
    packet_proc_def+='}'
    fout.writelines(''+packet_proc_declaration+'\n')
    fout.writelines(packet_proc_def+'\n')
fout.writelines('\n')
fout.writelines('bool '+class_name+'Stub::PacketProc(SessionInfo sessionInfo, CRecvBuffer& buf)\n')
fout.writelines('{\n')
fout.writelines('\tshort packetType;\n')
fout.writelines('\tbuf>>packetType;\n')
fout.writelines('\tswitch(packetType)\n')
fout.writelines('\t{\n')
for i in range(0,len(every_func_name)):
    func_name=every_func_name[i]
    fout.writelines('\tcase '+'PKT_TYPE_'+func_name+':\n')
    fout.writelines('\t{\n')
    fout.writelines('\t\treturn '+'PacketProc'+func_name+'(sessionInfo,buf);\n')
    fout.writelines('\t\tbreak;\n')
    fout.writelines('\t}\n')

fout.writelines('\tdefault:\n')
fout.writelines('\t{\n')
fout.writelines('\t\tcout<<"Packet Type not exist error"<<endl;\n')
fout.writelines('\t\treturn false;\n')
fout.writelines('\t\tbreak;\n')
fout.writelines('\t}\n')
fout.writelines('\t}\n')
fout.writelines('}\n')
fout.close()

#RegisterClass 헤더작성
for register_class in registered_classes:
    fout= open(register_class+"Sample.h",'wt')
    fout.writelines('#pragma once\n')
    fout.writelines('#include "CSendBuffer.h"\n')
    fout.writelines('#include "CRecvBuffer.h"\n')
    fout.writelines('#include "MakeUnique.h"\n')
    fout.writelines("class "+register_class+"\n")
    fout.writelines("{\n")
    fout.writelines("\tfriend CSendBuffer& operator<<(CSendBuffer& buf, "+register_class+"& tmp);\n")
    fout.writelines("\tfriend CRecvBuffer& operator>>(CRecvBuffer& buf, UniquePtr<"+register_class+">& ptr);\n")
    fout.writelines("};\n")