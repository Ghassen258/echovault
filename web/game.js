import createEchoVault from './echovault.mjs';

const $ = id => document.getElementById(id);
const board = $('board');
const ctx = board.getContext('2d');
const surface = $('game-surface');
const overlay = $('board-overlay');
const overlayButton = $('overlay-action');
const prefersReducedMotion = matchMedia('(prefers-reduced-motion: reduce)').matches;
let engine = null;
let state = null;
let started = false;
let animationStart = performance.now();
let flashUntil = 0;
let lastLevel = -1;
let liveRegionMessage = '';

const colors = { cyan:'#ffd464', lime:'#b8ee82', purple:'#e2a0b1', orange:'#f0a364', red:'#fa6e55' };
const gateNames = { A:'ALPHA', B:'BETA', C:'GAMMA' };
const keyMap = {ArrowUp:'w',ArrowDown:'s',ArrowLeft:'a',ArrowRight:'d',w:'w',a:'a',s:'s',d:'d',r:'r',c:'c',n:'n',' ':'.'};
function showOverlay(index,title,description,actionLabel,callback) {
  $('overlay-index').textContent=index;
  $('overlay-title').textContent=title;
  $('overlay-description').textContent=description;
  overlay.hidden=false;
  overlayButton.hidden=!actionLabel;
  if(actionLabel){overlayButton.innerHTML='';overlayButton.append(document.createTextNode(actionLabel+'  '));const arrow=document.createElement('span');arrow.textContent='→';overlayButton.append(arrow);overlayButton.onclick=callback;}
}
function hideOverlay(){overlay.hidden=true;}
function refresh() {
  if(!engine) return;
  state=JSON.parse(engine.getState());
  $('level-title').textContent=state.title;
  $('level-subtitle').textContent=state.subtitle;
  $('stat-tick').textContent=String(state.ticks).padStart(3,'0');
  $('board-clock').textContent='T+ '+String(state.ticks).padStart(3,'0');
  $('stat-echo').innerHTML=state.echoes.length+' <small>/ 8</small>';
  $('stat-rewind').textContent=String(state.rewinds).padStart(2,'0');
  $('stat-status').textContent=state.won?'OPEN':state.finished?'CLEARED':'SEALED';
  $('stat-status').style.color=state.won?colors.lime:colors.orange;
  $('level-progress-label').textContent=String(state.levelIndex+1).padStart(2,'0')+' / '+String(state.levelCount).padStart(2,'0');
  document.querySelectorAll('.level-bar').forEach((bar,i)=>{
    bar.classList.toggle('active',i===state.levelIndex);
    bar.classList.toggle('completed',i<state.levelIndex);
  });
  $('rewind').disabled=state.won||state.finished||state.ticks===0||state.echoes.length>=8;
  $('wait').disabled=state.won||state.finished;
  const presentGates=new Set(state.grid.join('').match(/[ABC]/g)||[]);
  const gateList=$('gate-list');
  gateList.replaceChildren();
  for(const gate of ['A','B','C']){
    if(!presentGates.has(gate))continue;
    const open=state.gates[gate];
    const item=document.createElement('div');item.className='gate-row'+(open?' open':'');
    const letter=document.createElement('span');letter.className='gate-letter';letter.textContent=gate;
    const name=document.createElement('span');name.className='gate-label';name.textContent=gateNames[gate]+' CIRCUIT';
    const status=document.createElement('span');status.className='gate-condition';status.textContent=open?'● OPEN':'○ LOCKED';
    item.append(letter,name,status);gateList.append(item);
  }
  const message=state.message;
  if(message!==liveRegionMessage){$('message').textContent=message;liveRegionMessage=message;}
  if(lastLevel!==state.levelIndex){lastLevel=state.levelIndex;animationStart=performance.now();}
}
function execute(key) {
  if(!engine||!started||!overlay.hidden)return;
  const result=engine.command(key);
  refresh();
  if(result===2){surface.classList.remove('blocked');void surface.offsetWidth;surface.classList.add('blocked');}
  if(result===3){surface.classList.add('rewinding');setTimeout(()=>surface.classList.remove('rewinding'),300);}
  if(result===5){surface.classList.add('won');showOverlay('VAULT '+String(state.levelIndex+1).padStart(2,'0')+' / BREACHED','THE VAULT IS OPEN','Your echoes held the line. The extraction path is yours.',state.levelIndex+1<state.levelCount?'ENTER NEXT VAULT':'COMPLETE MISSION',()=>advance());}
  if(result===7){showOverlay('PARADOX RESOLVED','TIME BELONGS TO YOU','Three vaults. Every timeline accounted for. Excellent work.','PLAY AGAIN',()=>location.reload());}
}
function advance() {
  hideOverlay();surface.classList.remove('won');
  if(!engine)return;
  engine.command('n');refresh();
  if(state.finished)showOverlay('PARADOX RESOLVED','TIME BELONGS TO YOU','All three vaults cleared. Every echo served its purpose.','PLAY AGAIN',()=>location.reload());
}
function moveFromEvent(event){const target=event.target;if(target instanceof HTMLElement && target.closest('button,select,input,textarea,a'))return; if($('help-dialog').open)return;const key=event.key.length===1?event.key.toLowerCase():event.key;if(key==='h'||key==='?'){event.preventDefault();$('help-dialog').showModal();return;}const command=keyMap[key]??(key==='.'?'.':null);if(command){event.preventDefault();execute(command);}else if(key==='Enter'&&state?.won){event.preventDefault();advance();}}
document.addEventListener('keydown',moveFromEvent);
$('rewind').addEventListener('click',()=>execute('r'));
$('wait').addEventListener('click',()=>execute('.'));
$('restart').addEventListener('click',()=>{if(!engine||!started)return;if(state?.finished){location.reload();return;}hideOverlay();surface.classList.remove('won');engine.command('c');refresh();});
document.querySelectorAll('[data-move]').forEach(button=>button.addEventListener('click',()=>execute(button.dataset.move)));
const help=$('help-dialog');
$('help-button').addEventListener('click',()=>help.showModal());
$('close-help').addEventListener('click',()=>help.close());
$('understood').addEventListener('click',()=>help.close());
help.addEventListener('click',event=>{if(event.target===help)help.close();});

function rounded(x,y,w,h,r){ctx.beginPath();ctx.roundRect(x,y,w,h,Math.min(r,w/2,h/2));}
function rectangle(x,y,w,h,fill){ctx.fillStyle=fill;ctx.fillRect(x,y,w,h);}
function ring(x,y,r,color,width=2,alpha=1){ctx.globalAlpha=alpha;ctx.beginPath();ctx.arc(x,y,r,0,Math.PI*2);ctx.strokeStyle=color;ctx.lineWidth=width;ctx.stroke();ctx.globalAlpha=1;}
function diamond(x,y,size,color,fill=true){ctx.beginPath();ctx.moveTo(x,y-size);ctx.lineTo(x+size,y);ctx.lineTo(x,y+size);ctx.lineTo(x-size,y);ctx.closePath();if(fill){ctx.fillStyle=color;ctx.fill();}else{ctx.strokeStyle=color;ctx.lineWidth=2;ctx.stroke();}}
function measureBoard(w,h,rows){const cols=rows[0].length;const count=rows.length;const tile=Math.min((w-32)/cols,(h-50)/count,36);return{tile,ox:(w-cols*tile)/2,oy:(h-count*tile)/2-4,cols,count};}
function render(now) {
  const w=board.clientWidth,h=board.clientHeight;
  if(w<2||h<2)return;
  ctx.clearRect(0,0,w,h);
  const glow=ctx.createRadialGradient(w*.45,h*.45,5,w*.45,h*.45,Math.max(w,h)*.75);
  glow.addColorStop(0,'#49382a');glow.addColorStop(.6,'#2b241f');glow.addColorStop(1,'#201a18');
  rectangle(0,0,w,h,glow);
  const t=prefersReducedMotion?0:now*.001;
  for(let i=0;i<55;i++){
    const x=((i*91.73+17)%1000)/1000*w,y=((i*157.33+97)%1000)/1000*h;
    rectangle(x,y,1,1,`rgba(255,218,148,${.07+(Math.sin(t*1.1+i)*.035)})`);
  }
  if(!state?.grid)return;
  const rows=state.grid;
  const {tile,ox,oy,cols,count}=measureBoard(w,h,rows);
  ctx.save();
  ctx.shadowColor='#ffd87530';ctx.shadowBlur=35;
  rounded(ox-8,oy-8,tile*cols+16,tile*count+16,3);ctx.strokeStyle='#bb925850';ctx.lineWidth=1;ctx.stroke();ctx.restore();
  for(let y=0;y<count;y++)for(let x=0;x<cols;x++){
    const char=rows[y][x];const px=ox+x*tile,py=oy+y*tile;
    const border=tile*.08;
    if(char==='#'){
      rectangle(px+1,py+1,tile-2,tile-2,'#58412f');
      rectangle(px+border,py+border,tile-2*border,tile-2*border,'#382d25');
      rectangle(px+border,py+border,tile-2*border,Math.max(1,tile*.08),'#9e7646');
    }else{
      rectangle(px+.8,py+.8,tile-1.6,tile-1.6,(x+y)%2?'#372c24':'#3d3026');
      if(tile>16){rectangle(px+tile*.12,py+tile*.12,2,2,'#7d6047');}
    }
    const cx=px+tile/2,cy=py+tile/2;
    if(char==='S'){
      ring(cx,cy,tile*.25,colors.cyan,1.5,.55);ring(cx,cy,tile*.12,colors.cyan,1,.35);
    }else if(char==='E'){
      ctx.save();ctx.shadowBlur=17;ctx.shadowColor=colors.lime;ring(cx,cy,tile*.25+Math.sin(t*2)*tile*.025,colors.lime,2,.9);diamond(cx,cy,tile*.14,colors.lime,false);ctx.restore();
    }else if(char>='a'&&char<='c'){
      const active=state.gates[char.toUpperCase()];
      ctx.save();ctx.shadowBlur=active?16:4;ctx.shadowColor=colors.orange;
      diamond(cx,cy,tile*.23,active?'#ffe098':'#d77d4e',false);
      ring(cx,cy,tile*.095,active?colors.lime:colors.orange,2);
      ctx.restore();
      if(tile>18){ctx.font=`700 ${Math.max(7,tile*.21)}px monospace`;ctx.fillStyle=colors.orange;ctx.textAlign='center';ctx.fillText(char.toUpperCase(),cx,py+tile*.22);}
    }else if(char>='A'&&char<='C'){
      const open=state.gates[char];
      ctx.save();ctx.shadowColor=open?colors.lime:colors.red;ctx.shadowBlur=open?12:17;
      rectangle(px+tile*.17,py+tile*.05,tile*.66,tile*.9,open?'#a8ec8636':'#ee73587f');
      ctx.lineWidth=tile*.07;ctx.strokeStyle=open?colors.lime:colors.red;ctx.strokeRect(px+tile*.17,py+tile*.05,tile*.66,tile*.9);ctx.restore();
      if(open){rectangle(cx-1,py+tile*.13,2,tile*.74,'#d8ffbb66');}else{
        rectangle(px+tile*.28,cy-1,tile*.44,2,'#ffe9cfcc');
        rectangle(cx-1,py+tile*.27,2,tile*.46,'#ffe9cfcc');
      }
    }
  }
  const palette=[colors.purple,'#ffd4a2','#c5b1ff','#b8ec85','#f39eb7','#ffecbb','#eab9d3','#edd89d'];
  state.echoes.forEach((echo,i)=>{
    const pigment=palette[i%palette.length];
    const path=echo.path;
    ctx.beginPath();
    path.forEach(([x,y],j)=>{
      const cx=ox+(x+.5)*tile,cy=oy+(y+.5)*tile;
      if(j===0)ctx.moveTo(cx,cy);else ctx.lineTo(cx,cy);
    });
    ctx.lineWidth=Math.max(1,tile*.1);ctx.lineCap='round';ctx.lineJoin='round';ctx.globalAlpha=.16;ctx.strokeStyle=pigment;ctx.stroke();ctx.globalAlpha=1;
    const [x,y]=echo.at,cx=ox+(x+.5)*tile,cy=oy+(y+.5)*tile;
    ctx.save();ctx.shadowBlur=20;ctx.shadowColor=pigment;
    ring(cx,cy,tile*.32+Math.sin(t*2+i)*tile*.035,pigment,1.2,.55);
    diamond(cx,cy,tile*.23,pigment,false);
    rectangle(cx-tile*.09,cy-tile*.09,tile*.18,tile*.18,pigment);
    ctx.restore();
    if(tile>=17){ctx.font=`700 ${Math.max(9,tile*.31)}px monospace`;ctx.fillStyle='#fff';ctx.textAlign='center';ctx.fillText(String(i+1),cx,cy-tile*.39);}
  });
  const [playerX,playerY]=state.player;
  const px=ox+(playerX+.5)*tile,py=oy+(playerY+.5)*tile;
  ctx.save();ctx.shadowBlur=24;ctx.shadowColor=colors.cyan;
  ring(px,py,tile*.38+Math.sin(t*3)*tile*.025,colors.cyan,1.1,.55);
  rounded(px-tile*.225,py-tile*.225,tile*.45,tile*.45,Math.max(2,tile*.07));ctx.fillStyle=colors.cyan;ctx.fill();
  diamond(px,py,tile*.135,'#3c2c25',true);
  ctx.restore();
  if(tile>15){ctx.font=`700 ${Math.max(10,tile*.38)}px 'Barlow Condensed',sans-serif`;ctx.textAlign='center';ctx.fillStyle='#ffe9ac';ctx.fillText('YOU',px,py-tile*.56);}
  if(flashUntil>now){rectangle(0,0,w,h,'#ffe18015');}
}
function sizeCanvas(){const dpr=Math.min(devicePixelRatio||1,2);const w=Math.max(1,Math.round(board.clientWidth*dpr)),h=Math.max(1,Math.round(board.clientHeight*dpr));if(board.width!==w||board.height!==h){board.width=w;board.height=h;}ctx.setTransform(dpr,0,0,dpr,0,0);}
function frame(now){sizeCanvas();render(now);requestAnimationFrame(frame);}
requestAnimationFrame(frame);
(async()=>{
  try{
    engine=await createEchoVault({locateFile:path=>new URL(path,import.meta.url).href});
    if(typeof engine.getState!=='function'||typeof engine.command!=='function')throw Error('WASM exports were not initialized');
    $('engine-state').textContent='ENGINE ONLINE';refresh();
    showOverlay('MISSION / INITIATION','ENTER THE ECHOVAULT','Three impossible rooms. One rewind mechanic. Your past self is your only ally.','BEGIN INFILTRATION',()=>{started=true;hideOverlay();});
  }catch(error){console.error('EchoVault WebAssembly failed to load:',error);$('engine-state').textContent='ENGINE OFFLINE';showOverlay('BOOT / ERROR','TIMELINE UNAVAILABLE','Unable to initialize the C++ WebAssembly module. Refresh the page or verify that echovault.wasm was deployed.','RETRY CONNECTION',()=>location.reload());}
})();
