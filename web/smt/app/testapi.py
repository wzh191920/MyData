from ctypes import *
ll = windll.LoadLibrary
lib = ll("./MyDataAPId.dll")


PI = POINTER(c_int)
handle = PI(c_int(0))
err = lib.MyDataConnect(c_char_p("127.0.0.1".encode('utf-8')), 8000, handle)
print(err , handle.contents.value)
handle = handle.contents.value
#type
'''
p = c_int*9
pp = p(2,3)
print(sizeof(p)//4)
err = lib.MyDataAddType(handle, pp, sizeof(p)//4, c_char_p("a积外围分aS".encode('utf-8')))
print(err)

num = PI(c_int(0))
err = lib.MyDataGetTypeNum(handle, num)
print(err, num.contents.value)


class TypeInfo(Structure):
	_fields_ = [("id", c_int),("type1", c_longlong),("type2", c_longlong),("name", c_char*32)]
	
typeinfo = (TypeInfo*num.contents.value)()
err = lib.MyDataGetTypes(handle, typeinfo, num)
print(num.contents.value)
for info in typeinfo:
	print(info.id, info.type1, info.type2, info.name.decode('utf-8'))
err = lib.MyDataDeleteType(handle, 3)
print(err)
'''
#table
'''
err = lib.MyDataAddTable(handle, 3, c_char_p("表2".encode('utf-8')))
print(err)

num = PI(c_int(0))
err = lib.MyDataGetTableNum(handle, num)
print(err, num.contents.value)

class TableInfo(Structure):
	_fields_ = [("id", c_int),("type_id", c_int),("name", c_char*32)]
tableinfo = (TableInfo*num.contents.value)()
err = lib.MyDataGetTables(handle, tableinfo, num)
print(num.contents.value)
for info in tableinfo:
	print(info.id, info.type_id, info.name.decode())

err = lib.MyDataDeleteTable(handle, 1)
print(err)
'''
#point
'''
err = lib.MyDataAddPoint(handle, 2, c_char_p("标签点3".encode('utf-8')))
print(err)
'''
class PointInfo(Structure):
	_fields_ = [("id", c_int),("table_id", c_int),("name", c_char*32)]
	
num = PI(c_int(0))
err = lib.MyDataSearchPointsNum(handle, c_char_p("".encode('utf-8')), c_char_p("".encode('utf-8')), num)
print(err, num.contents.value)

pointinfo = (PointInfo*num.contents.value)()
err = lib.MyDataSearchPoints(handle, c_char_p("".encode('utf-8')), c_char_p("".encode('utf-8')), pointinfo, num)
print(err, num.contents.value)
for info in pointinfo:
	print(info.id, info.table_id, info.name.decode())
'''
err = lib.MyDataDeletePoint(handle, 4)
print(err)
'''