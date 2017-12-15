from ctypes import windll, POINTER, c_int, c_longlong, c_char, c_char_p, Structure, c_short, c_float,\
    c_double

ll = windll.LoadLibrary
lib = ll("MyDataAPI.dll")

PI = POINTER(c_int)
PCH = POINTER(c_char)
PSH = POINTER(c_short)
PLL = POINTER(c_longlong)
PF = POINTER(c_float)
PD = POINTER(c_double)
PPCH = POINTER(c_char*2048)
handle = int()

class TableInfo(Structure):
    _fields_ = [("id", c_int),("type_id", c_int),("name", c_char*32)]
    
class TypeInfo(Structure):
    _fields_ = [("id", c_int),("type", c_char*16),("name", c_char*32),("type_names", c_char*512)]
    
class PointInfo(Structure):
    _fields_ = [("id", c_int),("table_id", c_int),("name", c_char*32)]
    def to_json(self):
        json_point = {
            "id":self.id,
            "table_id":self.table_id,
            "name":self.name.decode('utf-8')
        }
        return json_point
    
class Serializer(Structure):
    _fields_ = [("type", c_char*16), ("offset", c_int), ("buf", c_char*2048), ("data_pos", c_int)]
    
class DataFileInfo(Structure):
    _fields_ = [("start_time", c_longlong),("end_time", c_longlong),("file_size", c_longlong),
    ("using_rate", c_float), ("filename", c_char*32)]
    
def MydataConnect(logger):
    handle_tmp = PI(c_int(0))
    err = lib.MyDataConnect(c_char_p("127.0.0.1".encode('utf-8')), 8182, handle_tmp)
    if (err):
        logger.error('连接Mydata数据库失败, %d', err)
        return False
    global handle
    handle = handle_tmp.contents.value
    return True

PTYPE = POINTER(TypeInfo)
DataTypeMap = {1:'char', 2:'int16', 3:'int32', 4:'int64', 5:'float', 6:'double', 7:'string'}

def Typeid2Typestr(type_infos):
    typestrs = []

    for type_info in type_infos:
        strname = type_info.type_names.decode()
        strname = strname.split(';')
        typestr = []
        for i, data_type in enumerate(type_info.type):
            typestr.append((strname[i], DataTypeMap.get(data_type, '未定义'))) 
        typestrs.append(typestr) 
    return typestrs

def TransformReadableData(type_info, timestamps, sers, logger):
    readable_datas = list()
    err = 0
    for j, ser in enumerate(sers):
        readable_data = {0:timestamps[j]}
        for i, data_type in enumerate(type_info.type, start=1):
            if data_type == 1:
                ch = PCH(c_char(0))
                err = lib.ReadChar(ser, ch)
                if err:
                    logger.error('ReadChar fail, %d', err)
                    return None
                readable_data[i] = ch.contents.value.decode()
            elif data_type == 2:
                sh = PSH(c_short(0))
                err = lib.ReadShort(ser, sh)
                if err:
                    logger.error('ReadShort fail, %d', err)
                    return None
                readable_data[i] = sh.contents.value
            elif data_type == 3:
                pi = PI(c_int(0))
                err = lib.ReadInt(ser, pi)
                if err:
                    logger.error('ReadInt fail, %d', err)
                    return None
                readable_data[i] = pi.contents.value
            elif data_type == 4:
                pll = PLL(c_longlong(0))
                err = lib.ReadLongLong(ser, pll)
                if err:
                    logger.error('ReadLongLong fail, %d', err)
                    return None
                readable_data[i] = str(pll.contents.value)
            elif data_type == 5:
                pf = PF(c_float(0))
                err = lib.ReadFloat(ser, pf)
                if err:
                    logger.error('ReadFloat fail, %d', err)
                    return None
                readable_data[i] = pf.contents.value
            elif data_type == 6:
                pd = PD(c_double(0))
                err = lib.ReadDouble(ser, pd)
                if err:
                    logger.error('ReadDouble fail, %d', err)
                    return None
                readable_data[i] = pd.contents.value
            elif data_type == 7:
                ppch = PPCH((c_char*2048)())
                err = lib.ReadString(ser, ppch)
                if err:
                    logger.error('ReadString fail, %d', err)
                    return None
                
                readable_data[i] = ppch.contents.value.decode()
            else:
                logger.error('错误的类型, %d', data_type)
                return None
        readable_datas.append(readable_data)
    return {'retcode':0, 'length':len(type_info.type), 'readable_datas':readable_datas}