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
    return [this.doc.host, this.doc.port, this.doc.originatorAddress];
  },

  // При поступлении данных от плагина
  readStdin(stdin, readMap, holder) {
    let str = stdin.toString();
    if (!str) return;

    holder.emit('debug', 'plugin_' + this.id, str); // выдает в отладчик все строки

    // Нужно парсить входящие стороки и формировать значения каналов
    // Может быть подряд несколько строк
    const strArr = str.split('\n');

    const data = [];
    strArr.forEach(line => {
      const lineObj = processLine(line);
      if (lineObj) data.push(lineObj);
    });
    if (data.length) {
      const type = needUpsert(readMap, data) ? 'upsertChannels' : 'data';
      console.log('TYPE for send: '+type)
      return { type, data };
    }
  },

  // При отправке команды плагину
  // Пока передаю CMD:type adr val
  writeTele(chanObj) {
    // holder.emit('debug', 'plugin_' + this.id, 'writeTele '+util.inspect(chanObj));
    console.log('INFO: writeTele ' + util.inspect(chanObj));
    if (typeof chanObj == 'object' && chanObj.cmdtype) {
      return 'CMD: ' + chanObj.cmdtype + ' '+ chanObj.waddress + ' ' + chanObj.value;
    }
  }
};

function needUpsert(readMap, data) {
  for (const item of data) {
    if (!readMap.has(item.id)) return true;
  }
}

function processLine(str) {
  // console.log('processLine ' + str);
  str = allTrim(str);
  if (!str.startsWith('{') || !str.endsWith('}')) return;

  try {
    console.log('processLine try parse' + str);
    const obj = JSON.parse(str);
    const title = getTitle(obj);
    return { id: String(obj.address), title, ...obj };
    // return { ASDU: obj.ASDU, id: obj.address, value: obj.value, chstatus: obj.quality, ts: obj.ts };
  } catch (e) {
    console.log('ERROR: ' + util.inspect(e));
  }
}

function getTitle(obj) {
  let strtype = obj.type;
  if (!isNaN(strtype) && Number(obj.type)<10) {
    strtype = '0'+strtype;
  }
  return 'IObj' + strtype + obj.ASDU.substr(0,7) + '(' + obj.address + ')';
}

function allTrim(str) {
  return str && typeof str === 'string' ? str.replace(/^\s+/, '').replace(/\s+$/, '') : '';
}
