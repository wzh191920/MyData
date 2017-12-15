from flask import jsonify, request, g, abort, url_for, current_app
from . import api
import Mydata
from Mydata import lib, PI, handle, c_char_p, c_int, c_longlong

@api.route('/point', methods=['POST'])
def point():
    tid = int(request.form['tid'])
    point_name = request.form['point_name']
    num = PI(c_int(0))
    err = lib.MyDataSearchPointsNumByID(handle, tid, c_char_p(point_name.encode('utf-8')), num)
    if err:
        current_app.logger.error("获取标签点数量失败,%d", err)
        abort(404)
    pointinfo = (Mydata.PointInfo*num.contents.value)()
    err = lib.MyDataSearchPointsByID(handle, tid, c_char_p(point_name.encode('utf-8')), pointinfo, num)
    return jsonify({'point_infos':[info.to_json() for info in pointinfo]})

@api.route('/type', methods=['POST'])
def type_info():
    type_id = int(request.form['type_id'])
    typeinfo = (Mydata.TypeInfo*1)()
    err = lib.MyDataGetTypeByID(handle, type_id, typeinfo)
    if err :
        current_app.logger.error("获取指定类型失败,%d", err)
        abort(404)
    typestrs = Mydata.Typeid2Typestr(typeinfo)
    return jsonify({'typestrs':[{'name':element[0], 'typestr':element[1]} for element in typestrs[0]]})

@api.route('/nowdata', methods = ['POST'])
def nowdata():
    type_id = int(request.form['type_id'])
    ids = request.form['steps']
    ids = ids.split('H')
    num = PI(c_int(len(ids)))
    id_array = (c_int*len(ids))()
    try:
        for i in range(len(id_array)):id_array[i] = int(ids[i])
    except:
        abort(404)
    type_array = (c_int*len(ids))()
    for i in range(len(type_array)):type_array[i] = type_id
    timestamps = (c_longlong*len(ids))()
    sers = (Mydata.Serializer*len(ids))()
    err = lib.ReadNewDatas(handle, id_array, type_array, timestamps, sers, num)
    if err and err != -65275:
        current_app.logger.error("获取标签点最新数据失败,%d", err)
        abort(404)
    typeinfo = (Mydata.TypeInfo*1)()
    err = lib.MyDataGetTypeByID(handle, type_id, typeinfo)
    if err :
        current_app.logger.error("获取指定类型失败,%d", err)
        abort(404)
    readable_datas = Mydata.TransformReadableData(typeinfo[0], timestamps, sers, current_app.logger) 
    if readable_datas is None:
        abort(404)
    return jsonify(readable_datas)

@api.route('/alldatas', methods = ['POST'])
def alldatas():
    type_id = int(request.form['type_id'])
    point_id = int(request.form['point_id'])
    start_time = c_longlong(int(request.form['start_time']))
    end_time = c_longlong(int(request.form['end_time']))
    num = PI(c_int())
    lib.MyDataGetErrDesc.restype = c_char_p
    err = lib.ReadDatasNum(handle, point_id, start_time, end_time, num)
    if err :
        current_app.logger.error("获取数据数量失败,%d", err)
        return jsonify({'retcode':-1, 'errmsg':'获取数据数量失败,'+lib.MyDataGetErrDesc(err).decode('gbk')})
    
    timestamps = (c_longlong*num.contents.value)()
    sers = (Mydata.Serializer*num.contents.value)()
    
    err = lib.ReadDatas(handle, point_id, type_id, start_time, end_time, timestamps, sers, num)
    if err :
        current_app.logger.error("获取数据失败,%d", err)
        return jsonify({'retcode':-1, 'errmsg':'获取数据失败,'+lib.MyDataGetErrDesc(err).decode('gbk')})
    typeinfo = (Mydata.TypeInfo*1)()
    err = lib.MyDataGetTypeByID(handle, type_id, typeinfo)
    if err :
        current_app.logger.error("获取指定类型失败,%d", err)
        return jsonify({'retcode':-1, 'errmsg':'获取指定类型失败,'+lib.MyDataGetErrDesc(err).decode('gbk')})
    readable_datas = Mydata.TransformReadableData(typeinfo[0], timestamps, sers, current_app.logger) 
    if readable_datas is None:
        return jsonify({'retcode':-1, 'errmsg':'数据解析失败'})
    return jsonify(readable_datas)

@api.route('/addtype_result', methods = ['POST'])
def addtype_result():
    type_name = request.form['type_name']
    typeenums = request.form['typeenums']
    names = request.form['names']
    typeenums = typeenums.split('H')
    num = len(typeenums)
    typeenums_array = (c_int*num)()
    for i in range(len(typeenums)):typeenums_array[i] = int(typeenums[i])
    
    err = lib.MyDataAddType(handle, typeenums_array, num, c_char_p(type_name.encode()), c_char_p(names.encode()))
    if err:
        current_app.logger.error("添加类型失败,%d", err)
        lib.MyDataGetErrDesc.restype = c_char_p
        return jsonify({"ret":-1,'errmsg':lib.MyDataGetErrDesc(err).decode('gbk')})
    return jsonify({"ret":0})

@api.route('/delete_type', methods = ['POST'])
def delete_type():
    try:
        type_id = int(request.form['type_id'])
    except:
        type_id = -1
    err = lib.MyDataDeleteType(handle, type_id)
    if err:
        current_app.logger.error("删除类型失败,%d", err)
        lib.MyDataGetErrDesc.restype = c_char_p
        return jsonify({"ret":-1,'errmsg':lib.MyDataGetErrDesc(err).decode('gbk')})
    return jsonify({"ret":0})

@api.route('/add_table', methods = ['POST'])
def add_table():
    try:
        type_id = int(request.form['table_type_id'])
    except:
        type_id = -1
    name = request.form['name']
    err = lib.MyDataAddTable(handle, type_id, c_char_p(name.encode()))
    if err:
        current_app.logger.error("添加表失败,%d", err)
        lib.MyDataGetErrDesc.restype = c_char_p
        return jsonify({"ret":-1,'errmsg':lib.MyDataGetErrDesc(err).decode('gbk')})
    return jsonify({"ret":0})

@api.route('/delete_table', methods = ['POST'])
def delete_table():
    try:
        table_id = int(request.form['table_id'])
    except:
        table_id = -1
    err = lib.MyDataDeleteTable(handle, table_id)
    if err:
        current_app.logger.error("删除表失败,%d", err)
        lib.MyDataGetErrDesc.restype = c_char_p
        return jsonify({"ret":-1,'errmsg':lib.MyDataGetErrDesc(err).decode('gbk')})
    return jsonify({"ret":0})

@api.route('/get_tables', methods = ['POST'])
def get_tables():
    table_num = PI(c_int(0))
    err = lib.MyDataGetTableNum(handle, table_num)
    if err:
        current_app.logger.error("获取表数量失败,%d", err)
    table_info = (Mydata.TableInfo*table_num.contents.value)()
    err = lib.MyDataGetTables(handle, table_info, table_num)
    if err:
        current_app.logger.error("获取表信息失败,%d", err)
    tables = []
    for info in table_info:
        tables.append({"id":info.id, "type_id":info.type_id, "name":info.name.decode()})
    return jsonify({"tables":tables})

@api.route('/create_file', methods = ['POST'])
def create_file():
    try:
        size = int(request.form['size'])
    except:
        size = -1
    name = request.form['name']
    start_time = int(request.form['start_time'])
    end_time = int(request.form['end_time'])
    llstart = c_longlong(start_time)
    llend = c_longlong(end_time)
    llsize = c_longlong(size)
    err = lib.MyDataCreateFile(handle, c_char_p(name.encode()), llstart, llend, llsize)
    if err:
        current_app.logger.error("创建文件失败,%d", err)
        lib.MyDataGetErrDesc.restype = c_char_p
        return jsonify({"ret":-1,'errmsg':lib.MyDataGetErrDesc(err).decode('gbk')})
    return jsonify({"ret":0})

@api.route('/omit_file', methods = ['POST'])
def omit_file():
    try:
        start_time = int(request.form['start_time'])
    except:
        start_time = 0
    name = request.form['name']
    err = lib.MyDataDeleteFile(handle, c_char_p(name.encode()), c_longlong(start_time))
    if err:
        current_app.logger.error("排除文件失败,%d", err)
        lib.MyDataGetErrDesc.restype = c_char_p
        return jsonify({"ret":-1,'errmsg':lib.MyDataGetErrDesc(err).decode('gbk')})
    return jsonify({"ret":0})

@api.route('/rebuild_index', methods = ['POST'])
def rebuild_index():
    try:
        start_time = int(request.form['start_time'])
    except:
        start_time = 0
    name = request.form['name']
    err = lib.MyDataRebuildIndex(handle, c_char_p(name.encode()), c_longlong(start_time))
    if err:
        current_app.logger.error("重建索引失败,%d", err)
        lib.MyDataGetErrDesc.restype = c_char_p
        return jsonify({"ret":-1,'errmsg':lib.MyDataGetErrDesc(err).decode('gbk')})
    return jsonify({"ret":0})

@api.route('/add_file', methods = ['POST'])
def add_file():
    name = request.form['name']
    err = lib.MyDataAddFile(handle, c_char_p(name.encode()))
    if err:
        current_app.logger.error("添加文件失败,%d", err)
        lib.MyDataGetErrDesc.restype = c_char_p
        return jsonify({"ret":-1,'errmsg':lib.MyDataGetErrDesc(err).decode('gbk')})
    return jsonify({"ret":0})