/**
 * Функции - адаптеры
 *  - Каждая функция добавляется как метод плагина
 *    => В теле функции доступен объект плагина как this
 *       this.id - ID экземпляра
 *       this.doc = {  // Параметры экземпляра плагина
 *          ...
 *          host: '192.0.0.1',
 *          port: '2404'
 *       }
 */
const util = require('util');

module.exports = {
  // Отдает аргументы для командной строки
  getArgs() {
    return [this.doc.host, this.doc.port, this.doc.originatorAddress,
    this.doc.k, this.doc.w, this.doc.t0, this.doc.t1, this.doc.t2, this.doc.t3];
  },

  // При поступлении данных от плагина
  readStdin(stdin, readMap, holder) {
    if (this.readMapCur == undefined) this.readMapCur = {};
    if (this.readChunk == undefined) this.readChunk = '';
    if (Object.keys(this.readMapCur).length != readMap.size) {
      this.readMapCur = {};
      readMap.forEach((value, key) => {
        //this.readMap[value.ioObjMtype + '_' + value.objAdr] = key
        if (this.readMapCur[value.objAdr] == undefined) this.readMapCur[value.objAdr] = [];
        this.readMapCur[value.objAdr].push(value);
      })
    }

    let str = stdin.toString();
    if (str.substr(-1) != '\n') {
      this.readChunk += str;
      return { type: 'data', data: [] };
    } else {
      this.readChunk += str;
    }
    if (!this.readChunk) return;
    //holder.emit('debug', 'plugin_' + this.id, 'readMap = '+util.inspect(Object.keys(this.readMap)));
    holder.emit('debug', 'plugin_' + this.id, this.readChunk); // выдает в отладчик все строки

    // Нужно парсить входящие стороки и формировать значения каналов
    // Может быть подряд несколько строк
    const strArr = this.readChunk.split('\n');
    this.readChunk = '';
    const data = [];
    strArr.forEach(line => {
      if (line) {
        const lineObj = processLine(line, this.readMapCur, this.doc.tzondevice);
        if (lineObj) data.push(...lineObj);
      }
    });
    if (data.length) {
      //const type = needUpsert(readMap, data) ? 'upsertChannels' : 'data';
      const type = 'data';
      //holder.emit('debug', 'plugin_' + this.id, 'data= '+util.inspect(data));
      return { type, data };
    }
  },

  // При отправке команды плагину
  // Пока передаю CMD:type adr val selCmd ql ts
  writeTele(chanObj, holder) {
    holder.emit('debug', 'plugin_' + this.id, 'writeTele ' + util.inspect(chanObj));
    //console.log('INFO: writeTele ' + util.inspect(chanObj));
    if (typeof chanObj == 'object' && chanObj.ioObjCtype) {
      return 'CMD: ' + chanObj.ioObjCtype + ' ' + chanObj.objAdr + ' ' + chanObj.value + ' ' + chanObj.selCmd + ' ' + chanObj.ql + ' ' + Date.now();
    }
  }
};

function needUpsert(readMap, data) {
  for (const item of data) {
    if (!readMap.has(item.id)) return true;
  }
}

function processLine(str, readMap, tzondevice) {
  // console.log('processLine ' + str);
  str = allTrim(str);
  if (!str.startsWith('{') || !str.endsWith('}')) return;

  try {
    const obj = JSON.parse(str);
    const title = getTitle(obj);
    const arr = [];
    //return { id: readMap[obj.type + '_' + obj.address], title, ...obj };
    //return { id: readMap[obj.address], title, ...obj };
    if (tzondevice && Number(obj.type) > 15) {
      obj.ts = obj.ts + Number(tzondevice) * (-3600000);
    }
    if (readMap[obj.address]) {
      readMap[obj.address].forEach(item => {
        if (item.bit) {
          obj.value = obj.value & Math.pow(2, Number(item.offset)) ? 1 : 0;
        }
        arr.push({ id: item._id, title, ASDU: obj.ASDU, value: obj.value, chstatus: obj.chstatus, ts: obj.ts })
      })
      return arr;
    } else return [];


  } catch (e) {
    console.log('ERROR: ' + util.inspect(e));
  }
}

function getTitle(obj) {
  let strtype = obj.type;
  if (!isNaN(strtype) && Number(obj.type) < 10) {
    strtype = '0' + strtype;
  }
  return 'IObj' + strtype + obj.ASDU.substr(0, 7) + '(' + obj.address + ')';
}

function allTrim(str) {
  return str && typeof str === 'string' ? str.replace(/^\s+/, '').replace(/\s+$/, '') : '';
}
