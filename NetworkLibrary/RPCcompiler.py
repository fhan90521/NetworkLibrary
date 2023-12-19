#-*- encoding: utf-8 -*-
fin=open("C:\\Users\ghkdd\Desktop\\Procademy\\NetworkLibrary\\NetworkLibrary\\RPCreference.txt","rt",encoding='utf-8')
name_and_typenum=[]
every_func_name=[]
every_func_parameters_type=[]
every_func_parameters_name=[]
designated_pkt_type=dict()
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
        pkt_num=line.split(':')[-1]
        line = line.split(':')[0]
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
        if '*' in parameter[-1]:
            parameter[-1]=parameter[-1].replace('*','')
            parameter[-2]+='*'
        one_func_parameters_name.append(parameter[-1])
        type=''
        for i in range(0,len(parameter)-1):
            type+= parameter[i]
            if(i != len(parameter)-2):
                type+= ' '
        one_func_parameters_type.append(type)
    every_func_parameters_type.append(one_func_parameters_type)
    every_func_parameters_name.append(one_func_parameters_name)
fin.close()

#Proxy.h 작성
declaration_tails=[]
fout= open("C:\\Users\ghkdd\Desktop\\Procademy\\NetworkLibrary\\NetworkLibrary\\Proxy.h",'wt')
fout.writelines('#pragma once'+'\n')
fout.writelines('#include "Session.h"\n')
fout.writelines('#include ' +'"IOCPServer.h"'+'\n')
fout.writelines('#include ' +'<list>'+'\n')
fout.writelines('#include "PKT_TYPE.h"\n')
fout.writelines('using namespace std;\n')
#fout.writelines('namespace ' +name_and_typenum[0]+'\n')
#fout.writelines('{\n')
fout.writelines('class Proxy\n')
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
        tail+=' '
        tail+=parameters_type[j]
        if '*' not in parameters_type[j]:
            tail+='&'
        tail+=' '
        tail+=parameters_name[j]
        if(j != len(parameters_name)-1):
            tail+=','
    tail+=' )'
    declaration_tails.append(tail)
    fout.writelines('\tvoid '+func_name+'(SessionInfo sessionInfo,'+tail+';\n')
    fout.writelines('\tvoid '+func_name+'Post(SessionInfo sessionInfo,'+tail+';\n')
    fout.writelines('\tvoid '+func_name+'(list<SessionInfo>& sessionInfoList,'+tail+';\n')
    fout.writelines('\tvoid '+func_name+'Post(list<SessionInfo>& sessionInfoList,'+tail+';\n')
fout.writelines('\tProxy(IOCPServer* pServer)\n')
fout.writelines('\t{\n')
fout.writelines('\t\t_pServer=pServer;\n')
fout.writelines('\t}\n')
fout.writelines('};\n')
#fout.writelines('}\n')
fout.close()



#Proxy.cpp 작성
fout= open("C:\\Users\ghkdd\Desktop\\Procademy\\NetworkLibrary\\NetworkLibrary\\Proxy.cpp",'wt')
fout.writelines('#include "Proxy.h"\n')
fout.writelines('#include "PKT_TYPE.h"\n')
#fout.writelines('namespace ' +name_and_typenum[0]+'\n')
#fout.writelines('{\n')
for i in range(0,len(declaration_tails)):
    func_def='{\n'
    func_def+='\tCSerialBuffer* pBuf = new CSerialBuffer;\n'
    func_def+='\tpBuf->IncrementRefCnt();\n'
    func_def+='\ttry\n'
    func_def+= '\t{\n'
    func_def+='\t\t*pBuf'
    for j in range(0,len(every_func_parameters_name[i])):
        func_def+=' << '
        func_def+=every_func_parameters_name[i][j]
    func_def+=';\n'
    func_def+='\t}\n'
    func_def+='\tcatch(int useSize)\n'
    func_def+='\t{\n'
    func_def+='\t}\n'

    fout.writelines('void Proxy::'+every_func_name[i]+'(SessionInfo sessionInfo,'+declaration_tails[i]+'\n')
    fout.writelines(func_def)
    fout.writelines('\t_pServer->Unicast(sessionInfo, pBuf);\n')
    fout.writelines('\tpBuf->DecrementRefCnt();\n}\n')

    fout.writelines('void Proxy::'+every_func_name[i]+'Post(SessionInfo sessionInfo,'+declaration_tails[i]+'\n')
    fout.writelines(func_def)
    fout.writelines('\t_pServer->UnicastPost(sessionInfo, pBuf);\n')
    fout.writelines('\tpBuf->DecrementRefCnt();\n}\n')
    
    fout.writelines('void Proxy::'+every_func_name[i]+'(list<SessionInfo>& sessionInfoList,'+declaration_tails[i]+'\n')
    fout.writelines(func_def)
    fout.writelines('\tfor(SessionInfo sessionInfo: sessionInfoList)\n')
    fout.writelines('\t{\n')
    fout.writelines('\t\t_pServer->Unicast(sessionInfo, pBuf);\n')
    fout.writelines('\t}\n')
    fout.writelines('\tpBuf->DecrementRefCnt();\n')
    fout.writelines('}\n')

    fout.writelines('void Proxy::'+every_func_name[i]+'Post(list<SessionInfo>& sessionInfoList,'+declaration_tails[i]+'\n')
    fout.writelines(func_def)
    fout.writelines('\tfor(SessionInfo sessionInfo: sessionInfoList)\n')
    fout.writelines('\t{\n')
    fout.writelines('\t\t_pServer->UnicastPost(sessionInfo, pBuf);\n')
    fout.writelines('\t}\n')
    fout.writelines('\tpBuf->DecrementRefCnt();\n')
    fout.writelines('}\n')
#fout.writelines('}\n')
fout.close()

#PKT_TYPE.h 작성
name_and_typenum[1]=int(name_and_typenum[1])
fout= open("C:\\Users\ghkdd\Desktop\\Procademy\\NetworkLibrary\\NetworkLibrary\\PKT_TYPE.h",'wt')
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



#Stub.h 작성
fout= open("C:\\Users\ghkdd\Desktop\\Procademy\\NetworkLibrary\\NetworkLibrary\\Stub.h",'wt')
fout.writelines('#pragma once'+'\n')
#fout.writelines('#include "Proxy.h"\n')
fout.writelines('#include "Session.h"\n')
fout.writelines('#include "CMirrorBuffer.h"\n')
fout.writelines('#include "PKT_TYPE.h"\n')
#fout.writelines('namespace ' +name_and_typenum[0]+'\n')
#fout.writelines('{\n')
fout.writelines('class Stub\n')
fout.writelines('{\n')
#fout.writelines('protected:\n')
#fout.writelines('\tProxy* _pProxy=nullptr;\n')
fout.writelines('public:\n')
for i in range(0,len(every_func_name)):
    func_name=every_func_name[i]
    parameters_name=every_func_parameters_name[i]
    parameters_type=every_func_parameters_type[i]
    packet_proc_declaration= "bool PacketProc"+func_name +'( SessionInfo sessionInfo, CMirrorBuffer& buf)'
    proc_declaration="virtual void Proc"+func_name+'( SessionInfo sessionInfo, '
    for j in range(0,len(parameters_name)):
        proc_declaration+=' '
        proc_declaration+=parameters_type[j]
        if '*' not in parameters_type[j]:
            proc_declaration+='&'
        proc_declaration+=' '
        proc_declaration+=parameters_name[j]
        if(j != len(parameters_name)-1):
            proc_declaration+=','
    proc_declaration+=' )'
    fout.writelines('\t'+packet_proc_declaration+';\n')
    fout.writelines('\t'+proc_declaration+'{}\n\n')
fout.writelines('\tbool PacketProc(SessionInfo sessionInfo, PKT_TYPE packetType, CMirrorBuffer& buf);\n')
#fout.writelines('\tvoid AttachProxy(Proxy* pProxy);\n')
fout.writelines('};\n')
#fout.writelines('}\n')
fout.close()

#Stub.cpp 작성
fout= open("C:\\Users\ghkdd\Desktop\\Procademy\\NetworkLibrary\\NetworkLibrary\\Stub.cpp",'wt')
fout.writelines('#include "Stub.h"\n')
fout.writelines('#include "IOCPServer.h"\n')
fout.writelines('#include <iostream>\n')
fout.writelines('using namespace std;\n')
#fout.writelines('namespace ' +name_and_typenum[0]+'\n')
#fout.writelines('{\n')
for i in range(0,len(every_func_name)):
    func_name=every_func_name[i]
    parameters_name=every_func_parameters_name[i]
    parameters_type=every_func_parameters_type[i]
    packet_proc_declaration= "bool Stub::PacketProc"+ func_name +'(SessionInfo sessionInfo, CMirrorBuffer& buf)'
    packet_proc_def='{\n'
    for j in range(0,len(parameters_name)):
       packet_proc_def+='\t'+parameters_type[j]+' '+parameters_name[j]+';\n'
    
    packet_proc_def+='\ttry\n'
    packet_proc_def+='\t{\n'

    packet_proc_def+='\t\tbuf'  
    for j in range(0,len(parameters_name)):
       packet_proc_def+=' >> '+parameters_name[j]
    packet_proc_def+=';\n'
    packet_proc_def+='\t}\n'
    packet_proc_def+='\tcatch(int useSize)\n'
    packet_proc_def+='\t{\n'
    packet_proc_def+='\t\t cout<<" PacketProc'+func_name+' error"<<endl;\n'
    packet_proc_def+='\t\t return false;\n'
    packet_proc_def+='\t}\n'
    packet_proc_def+='\tProc'+func_name+'( sessionInfo, '
    for j in range(0,len(parameters_name)):
        packet_proc_def+=parameters_name[j]
        if j!=len(parameters_name)-1:
            packet_proc_def+=', '
    packet_proc_def+=');\n'
    packet_proc_def+='\treturn true;\n'
    packet_proc_def+='}'
    fout.writelines(''+packet_proc_declaration+'\n')
    fout.writelines(packet_proc_def+'\n')
fout.writelines('\n')
fout.writelines('bool Stub::PacketProc(SessionInfo sessionInfo, PKT_TYPE packetType, CMirrorBuffer& buf)\n')
fout.writelines('{\n')
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

#fout.writelines('void Stub::AttachProxy(Proxy* pProxy)\n')
#fout.writelines('{\n')
#fout.writelines('\t_pProxy = pProxy;\n')
#fout.writelines('}\n')


#fout.writelines('}\n')
fout.close()


