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
#c++ 기본타입
basic_type = ["int","unsigned int","short", "unsigned short","long","unsigned long","long long","unsigned long long","bool","char","unsigned char","float","double","ULONG64", "LONG64"]
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
        line =line.split(':')
        if line[0]=='RegisterClass':
            registered_classes.append(line[1])
            continue
        line[-1] = line[-1].replace(' ','')
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
        name_and_typenum[1] = int(designated_pkt_type[every_func_name[i]])+1
    else:
        pkt_type_def += str(name_and_typenum[1])
        name_and_typenum[1]+=1
    pkt_type_def+=';\n\n'
    fout.writelines(pkt_type_def)    
#fout.writelines('}\n')
fout.close()

#ServerProxy.h 작성
declaration_tails=[]
fout= open(class_name+"ServerProxy.h",'wt')
fout.writelines('#pragma once'+'\n')
fout.writelines('#include "Session.h"\n')
fout.writelines('#include ' +'"IOCPServer.h"'+'\n')
fout.writelines('#include ' +'"MyStlContainer.h"'+'\n')
for registered_class in registered_classes:
    fout.writelines('#include "'+registered_class+'.h"\n')
#fout.writelines('using namespace std;\n')
#fout.writelines('namespace ' +name_and_typenum[0]+'\n')
#fout.writelines('{\n')
fout.writelines('class '+class_name+'ServerProxy\n')
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
        #print(parameters_type[j])
        if parameters_type[j] not in basic_type:
            tail+='&'        
        tail+=' '
        tail+=parameters_name[j]
    declaration_tails.append(tail)
    fout.writelines('\tvoid '+func_name+'(SessionInfo sessionInfo'+tail+', bool bDisconnect = false )'+';\n')
    fout.writelines('\tvoid '+func_name+'(List<SessionInfo>& sessionInfoList'+tail+', bool bDisconnect = false);\n\n')
fout.writelines('\t'+class_name+'ServerProxy(IOCPServer* pServer)\n')
fout.writelines('\t{\n')
fout.writelines('\t\t_pServer=pServer;\n')
fout.writelines('\t}\n')
fout.writelines('};\n')
#fout.writelines('}\n')
fout.close()



#ServerProxy.cpp 작성
fout= open(class_name+"ServerProxy.cpp",'wt')
fout.writelines("#include " +' "'+class_name+'ServerProxy.h"\n')
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
    fout.writelines('void '+class_name+'ServerProxy::'+every_func_name[i]+'(SessionInfo sessionInfo'+declaration_tails[i]+', bool bDisconnect)\n')
    fout.writelines(func_def)
    fout.writelines('\t_pServer->Unicast(sessionInfo, pBuf, bDisconnect);\n')
    fout.writelines('\tpBuf->DecrementRefCnt();\n}\n')
    
    fout.writelines('void '+class_name+'ServerProxy::'+every_func_name[i]+'(List<SessionInfo>& sessionInfoList'+declaration_tails[i]+', bool bDisconnect)\n')
    fout.writelines(func_def)
    fout.writelines('\tfor(SessionInfo sessionInfo: sessionInfoList)\n')
    fout.writelines('\t{\n')
    fout.writelines('\t\t_pServer->Unicast(sessionInfo, pBuf, bDisconnect);\n')
    fout.writelines('\t}\n')
    fout.writelines('\tpBuf->DecrementRefCnt();\n')
    fout.writelines('}\n')
#fout.writelines('}\n')
fout.close()


#ClientProxy.h 작성
declaration_tails=[]
fout= open(class_name+"ClientProxy.h",'wt')
fout.writelines('#pragma once'+'\n')
fout.writelines('#include "Session.h"\n')
fout.writelines('#include ' +'"IOCPClient.h"'+'\n')
fout.writelines('#include ' +'"MyStlContainer.h"'+'\n')
for registered_class in registered_classes:
    fout.writelines('#include "'+registered_class+'.h"\n')
#fout.writelines('using namespace std;\n')
#fout.writelines('namespace ' +name_and_typenum[0]+'\n')
#fout.writelines('{\n')
fout.writelines('class '+class_name+'ClientProxy\n')
fout.writelines('{\n')
fout.writelines('private:\n')
fout.writelines('IOCPClient* _pClient;\n')
fout.writelines('public:\n')
for i in range(0,len(every_func_name)):
    func_name=every_func_name[i]
    parameters_name=every_func_parameters_name[i]
    parameters_type=every_func_parameters_type[i]
    tail=''
    for j in range(0,len(parameters_name)):
        if j!= 0:
            tail+=', '
        tail+=parameters_type[j]    
        if parameters_type[j] not in basic_type:
            tail+='&'      
        tail+=' '
        tail+=parameters_name[j]
    declaration_tails.append(tail)
    if len(tail) != 0 :
        fout.writelines('\tvoid '+func_name +'('+tail+', bool bDisconnect = false )'+';\n')
    else:
         fout.writelines('\tvoid '+func_name +'(bool bDisconnect = false )'+';\n')
fout.writelines('\t'+class_name+'ClientProxy(IOCPClient* pClient)\n')
fout.writelines('\t{\n')
fout.writelines('\t\t_pClient=pClient;\n')
fout.writelines('\t}\n')
fout.writelines('};\n')
#fout.writelines('}\n')
fout.close()



#ClientProxy.cpp 작성
fout= open(class_name+"ClientProxy.cpp",'wt')
fout.writelines("#include " +' "'+class_name+'ClientProxy.h"\n')
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
    if len(declaration_tails[i]) != 0:
        fout.writelines('void '+class_name+'ClientProxy::'+every_func_name[i]+'('+declaration_tails[i]+', bool bDisconnect)\n')
    else:
        fout.writelines('void '+class_name+'ClientProxy::'+every_func_name[i]+'(bool bDisconnect)\n')
    fout.writelines(func_def)
    fout.writelines('\t_pClient->Unicast(pBuf, bDisconnect);\n')
    fout.writelines('\tpBuf->DecrementRefCnt();\n}\n')
#fout.writelines('}\n')
fout.close()

for i in range(0,len(every_func_name)):
    func_name=every_func_name[i]
    parameters_name=every_func_parameters_name[i]
    parameters_type=every_func_parameters_type[i]
    for j in range(0,len(parameters_name)):
        if parameters_type[j] in registered_classes:
            parameters_name[j]=parameters_name[j]+"Ptr"

#ServerStub.h 작성
fout= open(class_name+"ServerStub.h",'wt')
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
fout.writelines('class '+class_name+'ServerStub\n')
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
            if parameters_type[j] not in basic_type:
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

#ServerStub.cpp 작성
fout= open(class_name+"ServerStub.cpp",'wt')
fout.writelines("#include " +' "'+class_name+'ServerStub.h"\n ')
fout.writelines('#include "IOCPServer.h"\n')
fout.writelines('#include <iostream>\n')
fout.writelines('#include "Log.h"\n')
fout.writelines('using namespace std;\n')
#fout.writelines('namespace ' +name_and_typenum[0]+'\n')
#fout.writelines('{\n')
for i in range(0,len(every_func_name)):
    func_name=every_func_name[i]
    parameters_name=every_func_parameters_name[i]
    parameters_type=every_func_parameters_type[i]
    packet_proc_declaration= "bool "+class_name+"ServerStub::PacketProc"+ func_name +'(SessionInfo sessionInfo, CRecvBuffer& buf)'
    packet_proc_def='{\n'
    for j in range(0,len(parameters_name)):
        if parameters_type[j] in registered_classes:
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
    packet_proc_def+='\t\t Log::LogOnFile(Log::DEBUG_LEVEL, "PacketProc'+func_name+' error\\n");\n'
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
fout.writelines('bool '+class_name+'ServerStub::PacketProc(SessionInfo sessionInfo, CRecvBuffer& buf)\n')
fout.writelines('{\n')
fout.writelines('\tshort packetType;\n')
fout.writelines('\ttry\n')
fout.writelines('\t{\n')
fout.writelines('\t\tbuf>>packetType;\n')
fout.writelines('\t}\n')
fout.writelines('\tcatch(int remainSize)\n')
fout.writelines('\t{\n')
fout.writelines('\t\t return false;\n')
fout.writelines('\t}\n')
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
fout.writelines('\t\tLog::LogOnFile(Log::DEBUG_LEVEL,"Packet Type not exist error\\n");\n')
fout.writelines('\t\treturn false;\n')
fout.writelines('\t\tbreak;\n')
fout.writelines('\t}\n')
fout.writelines('\t}\n')
fout.writelines('}\n')
fout.close()
#ClientStub 작성
#ClientStub.h 작성
fout= open(class_name+"ClientStub.h",'wt')
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
fout.writelines('class '+class_name+'ClientStub\n')
fout.writelines('{\n')
#fout.writelines('protected:\n')
#fout.writelines('\tProxy* _pProxy=nullptr;\n')
fout.writelines('public:\n')
for i in range(0,len(every_func_name)):
    func_name=every_func_name[i]
    parameters_name=every_func_parameters_name[i]
    parameters_type=every_func_parameters_type[i]
    packet_proc_declaration= "bool PacketProc"+func_name +'(CRecvBuffer& buf)'
    proc_declaration="virtual void Proc"+func_name+'('
    for j in range(0,len(parameters_name)):
        if j!= 0:
            proc_declaration+=', '
        if parameters_type[j] in registered_classes:
            proc_declaration+="UniquePtr<"+parameters_type[j]+'>& '
        else:
            proc_declaration+=parameters_type[j]
            if parameters_type[j] not in basic_type:
                proc_declaration+="&"    
        proc_declaration+=' '
        proc_declaration+=parameters_name[j]
    proc_declaration+=' )'
    fout.writelines('\t'+packet_proc_declaration+';\n')
    fout.writelines('\t'+proc_declaration+'{}\n\n')
fout.writelines('\tbool PacketProc(CRecvBuffer& buf);\n')
#fout.writelines('\tvoid AttachProxy(Proxy* pProxy);\n')
fout.writelines('};\n')
#fout.writelines('}\n')
fout.close()

#ClientStub.cpp 작성
fout= open(class_name+"ClientStub.cpp",'wt')
fout.writelines("#include " +' "'+class_name+'ClientStub.h"\n ')
fout.writelines('#include <iostream>\n')
fout.writelines('#include "Log.h"\n')
fout.writelines('using namespace std;\n')
#fout.writelines('namespace ' +name_and_typenum[0]+'\n')
#fout.writelines('{\n')
for i in range(0,len(every_func_name)):
    func_name=every_func_name[i]
    parameters_name=every_func_parameters_name[i]
    parameters_type=every_func_parameters_type[i]
    packet_proc_declaration= "bool "+class_name+"ClientStub::PacketProc"+ func_name +'(CRecvBuffer& buf)'
    packet_proc_def='{\n'
    for j in range(0,len(parameters_name)):
        if parameters_type[j] in registered_classes:
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
    packet_proc_def+='\t\t Log::LogOnFile(Log::DEBUG_LEVEL, "PacketProc'+func_name+' error\\n");\n'
    packet_proc_def+='\t\t return false;\n'
    packet_proc_def+='\t}\n'
    packet_proc_def+='\tProc'+func_name+'('
    for j in range(0,len(parameters_name)):
        if j!= 0:
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
fout.writelines('bool '+class_name+'ClientStub::PacketProc(CRecvBuffer& buf)\n')
fout.writelines('{\n')
fout.writelines('\tshort packetType;\n')
fout.writelines('\ttry\n')
fout.writelines('\t{\n')
fout.writelines('\t\tbuf>>packetType;\n')
fout.writelines('\t}\n')
fout.writelines('\tcatch(int remainSize)\n')
fout.writelines('\t{\n')
fout.writelines('\t\t return false;\n')
fout.writelines('\t}\n')
fout.writelines('\tswitch(packetType)\n')
fout.writelines('\t{\n')
for i in range(0,len(every_func_name)):
    func_name=every_func_name[i]
    fout.writelines('\tcase '+'PKT_TYPE_'+func_name+':\n')
    fout.writelines('\t{\n')
    fout.writelines('\t\treturn '+'PacketProc'+func_name+'(buf);\n')
    fout.writelines('\t\tbreak;\n')
    fout.writelines('\t}\n')

fout.writelines('\tdefault:\n')
fout.writelines('\t{\n')
fout.writelines('\t\tLog::LogOnFile(Log::DEBUG_LEVEL,"Packet Type not exist error\\n");\n')
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