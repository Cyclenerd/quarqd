#!/usr/bin/python

try:
    import ant_messages
    messages=ant_messages.messages

    import ant_sport_messages
    messages+=ant_sport_messages.messages

    import quarq_messages
    messages+=quarq_messages.messages

except ImportError:
    pass

class MessageSet:
    def __init__(self):
        self.lines=[]
    
    def load(self, fd):
        self.lines=[]
        self.header=None
        for line in fd.readlines():
            if "#" in line:
                line=line.split("#")[0]
            line=line.strip()
            if not line: 
                continue
            
            parts=line.split()
            if None==self.header:
                self.header=parts
            else:
                line={}
                for h,d in zip(self.header,parts):
                    line[h]=d
                self.lines.append(line)

    def select_where(self,**kwargs):

        def checkline(l):
            for k in kwargs:
                if l[k]!="*" and l[k]!=kwargs[k]:
                    return False
            return True

        result=MessageSet()
        result.lines=[r for r in self.lines if checkline(r)]
        return result

    def __getitem__(self, item):
        return self.lines.__getitem__(item)

    def __len__(self):
        return len(self.lines)
    
    def select_uniq(self, keyword):
        result=[]
        for l in self.lines:
            if not l[keyword] in result:
                if not l[keyword]=="*":
                    result.append(l[keyword])
        return result

import sys
ms=MessageSet()
ms.load(sys.stdin)

#print ms.lines
#print [a['ant_channel_type'] for a in ms.select_where(ant_channel_type='power')]
#print ms.select_uniq('xml_message')

class CodePrinter(object):
    def __init__(self):
        self.indent=0
        self.spacing="    "        
    def p(self, line):
        print (self.spacing*self.indent)+line
        
cp=CodePrinter()

def print_xml_code():
    global messages
    am=messages

    cp.p("""/* Generated messages for XML generation.
  These functions return 1 if they print a message, 0 if they do not find a match. */

""")


    for sensor_type in ms.select_uniq('ant_channel_type'):

        sensor_messages=ms.select_where(ant_channel_type=sensor_type)

        cp.p("int xml_message_interpret_%s_broadcast(ant_channel_t *self, unsigned char *message) {"%sensor_type)
        
        # we want to test for messages in the order they appear 
        # (which is arranged specific first, general last)
        # here's a real ungly sort for that
        ant_message_types=sensor_messages.select_uniq('ant_message_type')
        ant_message_types2=[]
        for x in messages.keys(): # the keys are in order
            if x in ant_message_types:
                ant_message_types2.append(x)
                
        # dirty hack, heart rate messages are not well-defined
        if sensor_type=="heartrate":
            ant_message_types2=["heart_rate"]

        for ant_message_type in ant_message_types2:
            qm=sensor_messages.select_where(ant_message_type=ant_message_type)
            am=messages[ant_message_type]

            def ant_message_is_diff(am,datatypes):
                prereqs={}
                for ant_datatype in datatypes:
                    v=am.byname[ant_datatype]
                    for d in v.depends(): 
                        if am.byname[d].diff:
                            return True
                return False
            diff=ant_message_is_diff(am,[m['ant_message_dataname'] for m in qm])
            
            cp.p("if ( MESSAGE_IS_%s(message) ) {"%ant_message_type.upper())
            cp.indent += 1

            if diff:
                cp.p("static unsigned char last_messages[ANT_CHANNEL_COUNT][MESSAGE_LEN_%s];"%(ant_message_type.upper()))
                cp.p("unsigned char * last_message=last_messages[self->number];")
                cp.p("if (!(self->mi.first_time_%s)) {"%ant_message_type)
                cp.indent += 1

            # dirty hack for SRM 
            if ant_message_type.upper() == 'TORQUE_SUPPORT':
                cp.p("set_srm_offset(self->device_id, TORQUE_SUPPORT_OFFSET_TORQUE(message));")

            try:
                timediff=am.byname['timediff']
                #print "#define UPDATE_TIMEDIFF(s,m) update_timediff(s,"+(ant_message_type+'_timediff').upper()+'(m))'            
            except KeyError:
                #print "#define UPDATE_TIMEDIFF(s,m) // no timediff in message"
                pass

            for m in qm:
                prereqs={}
                xmls=[]
                printf=[]
                http_tests=[]

                xml="<%s id='%%s' timestamp='%%.2f' "%(m['xml_message'])
                printf=printf+["self->id",
                               "timestamp"]
                               
                ant_datatype=m['ant_message_dataname']

                if am.byname.has_key(ant_datatype):

                    v=am.byname[ant_datatype]

                    for d in v.depends(): prereqs[d]=True

                    if "typename" in dir(v):

                        dataformat={"float":"%.2f",
                                    "uint32":"%u",
                                    "uint8":"%u"}[v.typename]
                    else:
                        dataformat="%d"                                    
                        #print dataformat
                                
                else: # byname isn't there!
                    print am.byname.keys()
                    raise Exception,ant_datatype

                xml=xml+"%s='%s' "%(m['xml_message_dataname'],dataformat)

                dataname=(ant_message_type+'_'+ant_datatype).upper()+'(message)'
                printf.append(dataname)

                http_tests.append({'channel_type':ant_datatype,
                                   'format_string':dataformat,
                                   'name':dataname})

                xmls.append(xml+"/>")
                #cp.p("// prereq "+m['xml_message']+':'+repr(prereqs.keys()))

                def is_blanking(m):
                    bd=qm.select_uniq('blank_display')
                    if len(bd)>1: 
                        raise "conflicting information in message_format.txt"
                    return (bd[0]=="True")

                xmls=[x+'\\n' for x in xmls]

                xml="\\\n".join(xmls)                

                if ant_message_is_diff(am,prereqs.keys()):
                    compares=[]
                    for p in prereqs.keys():
                        start=am.byname[p].pos
                        end=start+am.byname[p].width
                        for r in range(start,end):
                            compares.append((r,p))

                    r=("if ("+
                       (" ||\n\t".join(["(message[%d] != last_message[%d] /* %s */)"%(c,c,k) for c,k in sorted(compares)]))+
                       ") {")
                    for l in r.split('\n'):
                        cp.p(l)
                    cp.indent+=1

                cp.p("XmlPrintf(\""+xml+"\",")
                cp.indent += 1
                r=(",\n".join(printf))+");"
                for l in r.split('\n'):
                    cp.p(l)
                cp.indent -= 1

                if ant_message_is_diff(am,prereqs.keys()):
                    cp.indent-=1
                    cp.p("}")


            if diff:
                cp.indent-=1
                cp.p("} else {")
                cp.indent+=1
                cp.p("self->mi.first_time_%s=0;"%ant_message_type)
                cp.indent-=1
                cp.p("}")
                cp.p("memcpy(last_message,message,MESSAGE_LEN_%s);"%(ant_message_type.upper()))
                
            cp.indent-=1

            #print "#undef UPDATE_TIMEDIFF"

            if cp.indent: raise Exception,cp.indent
            print
            print "} else",

        print "{ return 0; }"
        print "\treturn 1;\n}\n"

if "--debug.c" in sys.argv:
    print '''void ant_message_print_debug(unsigned char * message) {'''    
    for m in messages.keys():    
        message=messages[m]

        pos=0
        if_clauses=[]
        auto_vars=[]
        print_statements=[]
        
        print_statements.append('\n\tfprintf(stderr,"%s:\\n");'%(message.name))
        for v in message.values:
            if not v.match_value is None:
                if_clauses.append("( message[%d]==0x%02x )"%(pos, v.match_value))
                
            elif v.width_format=='B': # unsigned char
                auto_vars.append('\tunsigned char %s = message[%d];'%(v.name,pos))
                print_statements.append('\tfprintf(stderr,"\\t%s: %%d (0x%%02X)\\n",%s,%s);'%(v.name,v.name,v.name))
            elif v.width_format=='b': # signed char
                auto_vars.append('\tsigned char %s = message[%d];'%(v.name,pos))
                print_statements.append('\tfprintf(stderr,"\\t%s: %%d (0x%%02X)\\n",%s,%s);'%(v.name,v.name,v.name))
            elif v.width_format in ['H','h']: # unsigned short 
                msb_index,lsb_index={'>':(pos,pos+1),
                                     '<':(pos+1,pos)}[v.endian]
                c_type={'H':'unsigned short',
                        'h':'signed short'}[v.width_format]
                                         
                auto_vars.append('\t%s %s = (message[%d]<<8)+(message[%d]);'%(c_type,v.name,msb_index,lsb_index))
                print_statements.append('\tfprintf(stderr,"\\t%s: %%d\\n",%s);'%(v.name,v.name))
                                         
                
            elif v.name=='None':
                pass
            else: 
                raise

        statement=['// %s'%message.name,
                   'if (%s) {'%" && ".join(if_clauses),
                   '\n'.join(auto_vars),
                   '\n'.join(print_statements),
                   '} else']

        print '\n'.join(statement)
    
    
    print ''' { ; } \n } '''


elif "--header" in sys.argv:

    diff_messages=[]
    
    defines={}
    
    for m in messages.keys():    
        message=messages[m]

        pad_index=0
        
        matches={}

        message_defines=[]

        for v in message.values:

            name=v.name

            if "diff" in dir(v) and v.diff:
                #if 'prev_value' in dir(v):
                name+='_sum'                   
                if not message in diff_messages:
                    diff_messages.append(message)


            if not v.match_value is None:
                matches[v.pos]=v.match_value
                
            elif v.width_format=='B': # unsigned char
                message_defines.append((name,'((uint8_t)message[%d])'%(v.pos)))

            elif v.width_format=='b': # signed char
                message_defines.append((name,'((sint8_t)message[%d])'%(v.pos)))

            elif v.width_format in ['H','h']:
                c_type={'H':'uint16_t',
                        'h':'int16_t'}[v.width_format]
                        
                lsb,msb={'<':(v.pos,v.pos+1), #le
                         '>':(v.pos+1,v.pos)}[v.endian] #be
                         
                message_defines.append((name,'((%s)(message[%d]+(message[%d]<<8)))'%(c_type,lsb,msb)))
                
            elif v.name=='None':
                pass
            else: 
                raise Exception,repr(v.name)+repr(v.match_value)+repr(v.width_format)+repr(message)

        print "\n// defines for message '%s'"%message.name
        for n,v in message_defines:
            name=(message.name+'_'+n).upper()
            defines[name]=v
            print '#define %-40s %s'%(name+'(message)',v)

        matchexpression=' && '.join(["(message[%d]==0x%02X)"%(k,matches[k]) for k in matches.keys()])

        print "#define MESSAGE_IS_%s(message) (%s)"%(message.name.upper(),matchexpression)
        print "#define MESSAGE_LEN_%s (%d)"%(message.name.upper(),len(message))

    for m in messages.keys():    
        message=messages[m]

        message_defines=[]

        for v in message.values:
            
            if "diff" in dir(v) and v.diff:
                if v.width_format in ['B','H','h']:

                    c_type={'B':'uint8_t',
                            'H':'uint16_t',
                            'h':'int16_t'}[v.width_format]

                    name=(message.name+'_'+v.name).upper()
                    
                    message_defines.append((name,
                                            "((%s)(%s_SUM(message)-%s_SUM(last_message)))"%(c_type,name,name)))

                else: 
                    raise            

        def equation_mkmacro(e):
            import re
            x=re.split('([a-z].[a-z0-9_]+)',e)
            for i in range(1,len(x),2):
                x[i]=(message.name+'_'+x[i]).upper()+'(message)'
            return ''.join(x)

        for c in message.calculations:
            if c.typename in ['uint32','float']:
                name=(message.name+'_'+c.name).upper()
                typename={'float':'float',
                          'uint32':'uint32_t'}[c.typename]
                
                eq='((%s)(%s))'%(typename,equation_mkmacro(c.eq))

                # dirty hack for SRM messages
                if name=="CRANK_SRM_OFFSET":
                    eq="(get_srm_offset(self->device_id))"

                message_defines.append((name,eq))

        if len(message_defines):
            print ''.join(["\n#define %s(message) %s"%x for x in message_defines])

    print

    for m in messages.keys():    
        message=messages[m]

        pos=0
        pad_index=0
        
        send_args=[]
        sends=[]        

        for v in message.values:

            name=v.name

            if "diff" in dir(v) and v.diff:
                name+='_sum'                   

            if not v.match_value is None:
                sends.append(("0x%02x"%v.match_value,"d%d"%v.match_value))
                
            elif v.width_format=='B': # unsigned char
                send_args.append(('uint8_t',name))
                sends.append(('((uint8_t)%s)'%name,None))

            elif v.width_format=='b': # signed char
                send_args.append(('sint8_t',name))
                sends.append(('((sint8_t)%s)'%name,None))

            elif v.width_format in ['H','h']:
                c_type={'H':'uint16_t',
                        'h':'int16_t'}[v.width_format]
                        
                lsb,msb={'<':(pos,pos+1), #le
                         '>':(pos+1,pos)}[v.endian] #be
                         
                send_args.append((c_type,name))
                if v.endian=='<':
                    sends.append(('(((%s)%s)&0xff)'%(c_type,name),None))
                    sends.append(('(((%s)%s)>>8)'%(c_type,name),None))
                else:
                    sends.append(('(((%s)%s)>>8)'%(c_type,name),None))
                    sends.append(('(((%s)%s)&0xff)'%(c_type,name),None))
                
            elif v.name=='None':
                sends.append(('0','unknown/don\'t care'))
            else: 
                raise Exception,repr(v.name)+repr(v.match_value)+repr(v.width_format)+repr(message)

            pos+=v.width

        print "\n// defines for message '%s'"%message.name
        
        args=','.join([v for n,v in send_args])
        
        sends=[('0xa4','sync'), ('%d'%(pos-1),'length')]+sends

        print "#define SEND_MESSAGE_%s(%s) do {\\"%(message.name.upper(),args)
        tx=[]
        for s in sends:
            if s[1]==None:
                tx.append("\tTRANSMIT(%s);"%(s[0]))
            else:
                tx.append("\tTRANSMIT(%s); /* %s */"%s)
        tx.append("\tSEND_CHECK();") 
        for t in tx:
            print "%-60s\\"%t

        print "} while (0);\n"
 
    for m in ms.select_uniq('ant_channel_type'):
        print "int xml_message_interpret_%s_broadcast(ant_channel_t *self, unsigned char *message);"%m

elif '--header1' in sys.argv:
    print "typedef struct {"
    for m in ms.select_uniq('ant_message_type'):
        print "\tint first_time_%s;"%m
    print "} messages_initialization_t;"
    print
    
    print "#define INITIALIZE_MESSAGES_INITIALIZATION(x) \\"
    for m in ms.select_uniq('ant_message_type'):
        print "\tx.first_time_%s=1; \\"%m
    print
    
elif '--xml_messages' in sys.argv:
    print_xml_code()
 
elif '--sql' in sys.argv:
    for xml_message in ms.select_uniq('xml_message'):       
        xml_messages=ms.select_where(xml_message=xml_message)

        print "CREATE TABLE IF NOT EXISTS %s ("%xml_message,

        print "id TEXT, timestamp FLOAT,",

        for xml_message_dataname in xml_messages.select_uniq('xml_message_dataname'):
            message=xml_messages.select_where(xml_message_dataname=xml_message_dataname)

            data_types=message.select_uniq('sql_data_type')
            if len(data_types)>1:
                raise # Can't have two datatypes at once
            sql_data_type=data_types[0]
            
            print "%s %s"%(xml_message_dataname, sql_data_type),
        print ");"

                           
