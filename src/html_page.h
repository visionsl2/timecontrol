/**
 * HTML页面内容
 *
 * 使用R"rawliteral(...)"语法将HTML作为原始字符串字面量，
 * 这样可以避免Arduino预处理器将JavaScript代码中的"function"等关键字解析为C++代码。
 */

const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width">
<title>时间管控器</title>
<style>
body{font-family:Arial;max-width:600px;margin:20px auto;padding:20px;background:#f5f5f5}
h1{color:#333}
.card{background:white;padding:20px;margin:15px 0;border-radius:8px;box-shadow:0 2px 4px rgba(0,0,0,0.1)}
h2{margin-top:0;color:#e94560}
label{display:block;margin:10px 0 5px}
input,select{width:100%;padding:10px;border:1px solid #ddd;border-radius:4px;box-sizing:border-box}
button{background:#e94560;color:white;border:none;padding:12px 24px;border-radius:4px;cursor:pointer;margin:10px 5px 0 0}
button:hover{background:#d1364e}
button.secondary{background:#666}
.tag{display:inline-block;background:#00b894;color:white;padding:5px 10px;border-radius:4px;margin:3px;font-family:monospace}
.detected{color:#00b894;font-family:monospace}
.nav{margin:20px 0}
.nav a{margin-right:15px}
</style>
</head>
<body>
<h1>时间管控器</h1>
<div class="nav">
<button onclick="show('status')">状态</button>
<button onclick="show('config')">配置</button>
<button onclick="show('nfc')">NFC</button>
</div>

<!-- 状态页面 -->
<div id="status-page" class="card" style="display:none">
<h2>实时状态</h2>
<div id="current-state" style="font-size:24px;font-weight:bold;text-align:center;padding:20px;">加载中...</div>
<div id="nfc-detected" style="text-align:center;color:#666;margin:10px 0;">NFC检测中...</div>
<div id="timer-display" style="text-align:center;font-size:18px;margin:10px 0;"></div>
</div>

<!-- 配置页面 -->
<div id="config-page" class="card">
<h2>设置</h2>
<label>超时时长（分钟）</label>
<input type="number" id="timeout" value="30">
<label>WiFi SSID</label>
<input type="text" id="wifi_ssid" placeholder="WiFi名称">
<label>WiFi密码</label>
<input type="password" id="wifi_pass" placeholder="WiFi密码">
<label>通知URL</label>
<input type="text" id="notify_url" placeholder="http://...">
<button onclick="saveConfig()">保存配置</button>
</div>

<!-- NFC标签管理页面 -->
<div id="nfc-page" class="card" style="display:none">
<h2>NFC标签管理</h2>
<label>已注册标签</label>
<div id="tag-list"></div>
<label style="margin-top:20px">当前检测到的标签</label>
<div class="detected" id="detected-tag">等待检测...</div>
<button onclick="addTag()" style="background:#00b894">添加当前标签</button>
<button onclick="loadNFC()" class="secondary">重新扫描</button>
</div>

<script>
var api='/api';

var stateTimer=null;
/**
 * 加载状态信息
 */
function loadStatus(){
fetch(api+'/status').then(function(r){return r.json()}).then(function(d){
var s=document.getElementById('current-state');
var n=document.getElementById('nfc-detected');
var t=document.getElementById('timer-display');
var colors={IDLE:'#666',ACTIVE:'#00b894',COUNTING:'#fdcb6e',WARNING:'#e94560'};
s.textContent=d.state;
s.style.color=colors[d.state]||'#333';
n.textContent='检测到的NFC: '+(d.nfc||'无');
t.textContent=d.state==='COUNTING'?'已计时: '+d.elapsed:'';
});}

/**
 * 加载配置信息
 * GET /api/config 返回: {timeout, wifi_ssid, notify_url}
 */
function loadConfig(){
fetch(api+'/config').then(function(r){return r.json()}).then(function(d){
document.getElementById('timeout').value=d.timeout||30;
document.getElementById('wifi_ssid').value=d.wifi_ssid||'';
document.getElementById('notify_url').value=d.notify_url||'';});}

var statusInterval=null;
function show(p){
document.getElementById('config-page').style.display=p==='config'?'block':'none';
document.getElementById('nfc-page').style.display=p==='nfc'?'block':'none';
document.getElementById('status-page').style.display=p==='status'?'block':'none';
if(p==='nfc')loadNFC();
if(p==='status'){
loadStatus();
if(statusInterval)clearInterval(statusInterval);
statusInterval=setInterval(loadStatus,1000);
} else {
if(statusInterval){clearInterval(statusInterval);statusInterval=null;}
}};

/**
 * 保存配置
 * POST /api/config
 */
async function saveConfig(){
var t=document.getElementById('timeout').value||'30';
var s=document.getElementById('wifi_ssid').value||'';
var p=document.getElementById('wifi_pass').value||'';
var u=document.getElementById('notify_url').value||'';
var data='timeout='+t+'&wifi_ssid='+encodeURIComponent(s)+'&wifi_pass='+encodeURIComponent(p)+'&notify_url='+encodeURIComponent(u);
console.log('saveConfig:',data);
try {
var resp=await fetch(api+'/config',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:data});
console.log('saveConfig status:',resp.status);
var text=await resp.text();
console.log('saveConfig response:',text);
if(text.indexOf('reboot')>=0){
alert('配置已保存，设备将重启连接WiFi...');
} else {
alert(text.indexOf('ok')>=0?'已保存!':'保存失败');
}
} catch(e){console.error('saveConfig error:',e);alert('保存失败:'+e);}
// 延迟刷新让设备重启
setTimeout(loadConfig,3000);
}

/**
 * 加载NFC状态
 * GET /api/nfc 返回: {detected当前标签, tags已注册数组}
 */
function loadNFC(){
fetch(api+'/nfc').then(function(r){return r.json()}).then(function(d){
document.getElementById('detected-tag').textContent=d.detected||'未检测到标签';
var list=document.getElementById('tag-list');
list.innerHTML='';
for(var i=0;i<d.tags.length;i++){
var span=document.createElement('span');
span.className='tag';
span.innerHTML=d.tags[i]+' <span style="cursor:pointer;color:#ff6b6b" onclick="removeTag('+i+')">x</span>';
list.appendChild(span);}});}

/**
 * 添加当前检测到的标签到白名单
 * POST /api/nfc/add
 */
function addTag(){
var t=document.getElementById('detected-tag').textContent.trim();
console.log('addTag:',t);
if(t && t!='未检测到标签' && t!='' && t.indexOf('等待')==-1){
var x=new XMLHttpRequest();
x.open('POST',api+'/nfc/add',true);
x.setRequestHeader('Content-Type','application/x-www-form-urlencoded');
x.send('uid='+encodeURIComponent(t));
x.onload=function(){console.log('addTag response:',x.responseText);loadNFC();};}
else{alert('请先检测到NFC标签');}}

/**
 * 从白名单移除标签
 * POST /api/nfc/remove
 */
function removeTag(i){
var x=new XMLHttpRequest();
x.open('POST',api+'/nfc/remove',true);
x.send('index='+i);
x.onload=loadNFC;}

/**
 * 切换显示页面
 * @param p 'config' 或 'nfc'
 */
function show(p){
document.getElementById('config-page').style.display=p=='config'?'block':'none';
document.getElementById('nfc-page').style.display=p=='nfc'?'block':'none';
if(p=='nfc')loadNFC();}

// 初始化
loadConfig();
loadNFC();
setInterval(loadNFC,2000);
</script>
</body>
</html>
)rawliteral";