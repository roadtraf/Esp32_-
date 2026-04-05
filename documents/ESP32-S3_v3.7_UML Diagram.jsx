=======================
두 다이어그램 모두 실제 소스코드와 정확히 대응되도록 작성했습니다.
상태 머신 다이어그램 — StateMachine.cpp의 updateStateMachine() + changeState()를 그대로 반영했습니다. 
정상 사이클 6단계는 실선으로, 
WAIT_REMOVAL → ERROR (10s 타임아웃)·ERROR → 
IDLE (복구)·EMERGENCY → IDLE (복귀)는 빨간 점선으로 구분되고, 
과전류·비상정지 같은 글로벌 트리거는 좌하단 legends로 표시됩니다. 
상태를 클릭하면 해당 상태의 진입·종료 조건만 강조됩니다.
시퀀스 다이어그램 — loop()의 실제 호출 순서와 타이밍 주기(센서 100ms, PID 50ms, UI 200ms 등)를 
기준으로 정상 진공 사이클 전체를 8단계로 분할했습니다.
 changeState() 호출 시점은 밝은 파란색 행으로 강調되며, 
상단 버튼으로 원하시는 구간만 축소 확인할 수 있습니다.
≠==========================

import { useState } from "react";

/* ─── 디자인 토큰 ─── */
const T = {
  bg:     "#080c12",
  card:   "#0f1520",
  border: "#1a2736",
  t1:     "#e2e8f0",   // 밝은 텍스트
  t2:     "#94a3b8",   // 중간
  t3:     "#4a5568",   // 어두운
  cyan:   "#22d3ee",   // IDLE / accent
  green:  "#4ade80",   // 정상 진행 상태
  amber:  "#fbbf24",   // VACUUM_BREAK
  orange: "#fb923c",   // WAIT_REMOVAL
  red:    "#f87171",   // ERROR / EMERGENCY
  purple: "#c084fc",   // PID
  pink:   "#f472b6",   // UI
  sky:    "#60a5fa",   // Sensor
};
const MONO = "'SF Mono','Fira Code','Consolas',monospace";

/* ─── 상태 머신 정의 (StateMachine.cpp와 1:1) ─── */
const NODES = {
  IDLE:            { label:"IDLE",          sub:"대기",      col: T.cyan   },
  VACUUM_ON:       { label:"VACUUM_ON",     sub:"진공 작동", col: T.green  },
  VACUUM_HOLD:     { label:"VACUUM_HOLD",   sub:"진공 유지", col: T.green  },
  VACUUM_BREAK:    { label:"VACUUM_BREAK",  sub:"진공 해제", col: T.amber  },
  WAIT_REMOVAL:    { label:"WAIT_REMOVAL",  sub:"제거 대기", col: T.orange },
  COMPLETE:        { label:"COMPLETE",      sub:"완료",      col: T.green  },
  ERROR:           { label:"ERROR",         sub:"에러",      col: T.red    },
  EMERGENCY_STOP:  { label:"EMERGENCY",     sub:"비상정지",  col: T.red    },
};

/* 좌표 — 정상 사이클을 시계방향 배열, 에러/비상을 좌측 배치 */
const XY = {
  IDLE:           [400, 58],
  VACUUM_ON:      [590, 148],
  VACUUM_HOLD:    [648, 300],
  VACUUM_BREAK:   [538, 425],
  WAIT_REMOVAL:   [330, 438],
  COMPLETE:       [195, 330],
  ERROR:          [112, 182],
  EMERGENCY_STOP: [252, 62],
};

/* 엣지 정의 — from, to, 조건 텍스트, 에러 여부 */
const EDGES = [
  { from:"IDLE",           to:"VACUUM_ON",      cond:"limitSwitch ON\n/ START 명령",       err:false },
  { from:"VACUUM_ON",     to:"VACUUM_HOLD",    cond:"AUTO: onTime 만료\nPID: 목표압력 도달", err:false },
  { from:"VACUUM_HOLD",   to:"VACUUM_BREAK",   cond:"holdTime 만료",                      err:false },
  { from:"VACUUM_BREAK",  to:"WAIT_REMOVAL",   cond:"breakTime 만료",                     err:false },
  { from:"WAIT_REMOVAL",  to:"COMPLETE",       cond:"photoSensor OFF\n(제품 제거 감지)",   err:false },
  { from:"COMPLETE",      to:"IDLE",           cond:"1 s 후 자동 복귀",                   err:false },
  { from:"WAIT_REMOVAL",  to:"ERROR",          cond:"10 s 타임아웃",                      err:true  },
  { from:"ERROR",         to:"IDLE",           cond:"복구 성공",                          err:true  },
  { from:"EMERGENCY_STOP",to:"IDLE",           cond:"비상정지 복귀\n(SW HIGH)",           err:true  },
];

/* 글로벌 트리거 (모든 상태에서 즉시 전이) */
const GLOBALS = [
  { to:"ERROR",          cond:"과전류 감지 (current > 6 A)" },
  { to:"EMERGENCY_STOP", cond:"비상정지 SW (NC → LOW)"     },
];

/* ─── 시퀀스 다이어그램 정의 ─── */
const ACTORS = [
  { id:"loop",   name:"loop()",      col: T.cyan   },
  { id:"sensor", name:"Sensor",      col: T.sky    },
  { id:"sm",     name:"StateMachine",col: T.green  },
  { id:"ctrl",   name:"Control",     col: T.amber  },
  { id:"pid",    name:"PID",         col: T.purple },
  { id:"graph",  name:"TrendGraph",  col: T.orange },
  { id:"ui",     name:"UI",          col: T.pink   },
  { id:"sd",     name:"SD_Logger",   col: T.t3     },
];

/* 각 Phase = 하나의 상태 전이 단계 */
const PHASES = [
  { title:"① IDLE — 대기 루프 (100 ms)",
    msgs:[
      { f:"loop",   t:"sensor", txt:"readSensors()"              },
      { f:"sensor", t:"sensor", txt:"checkSensorHealth()"        },
      { f:"sensor", t:"sensor", txt:"validateParameters()"       },
      { f:"loop",   t:"sm",     txt:"updateStateMachine()"       },
      { f:"loop",   t:"ui",     txt:"updateUI()  [200 ms]"       },
    ]
  },
  { title:"② IDLE → VACUUM_ON",
    msgs:[
      { f:"loop",   t:"sensor", txt:"readSensors()\n→ limitSwitch = true" },
      { f:"loop",   t:"sm",     txt:"updateStateMachine()"       },
      { f:"sm",     t:"sm",     txt:"changeState(VACUUM_ON)",    hl:true },
      { f:"sm",     t:"graph",  txt:"initGraphData()"            },
      { f:"sm",     t:"ctrl",   txt:"control12VMain(true)"       },
      { f:"sm",     t:"ctrl",   txt:"controlPump(true, 200)"     },
    ]
  },
  { title:"③ VACUUM_ON — 진공 작동 루프",
    msgs:[
      { f:"loop",   t:"sensor", txt:"readSensors()  [100 ms]"    },
      { f:"loop",   t:"pid",    txt:"updatePID()    [50 ms]"     },
      { f:"pid",    t:"pid",    txt:"err = target − pressure"    },
      { f:"pid",    t:"ctrl",   txt:"controlPump(true, pwm)"     },
      { f:"loop",   t:"graph",  txt:"addGraphPoint()  [100 ms]"  },
      { f:"loop",   t:"ui",     txt:"updateUI()  [200 ms]"       },
    ]
  },
  { title:"④ VACUUM_ON → VACUUM_HOLD",
    msgs:[
      { f:"loop",   t:"sm",     txt:"updateStateMachine()"       },
      { f:"sm",     t:"sm",     txt:"changeState(VACUUM_HOLD)",  hl:true },
      { f:"sm",     t:"pid",    txt:"resetPID()"                 },
    ]
  },
  { title:"⑤ VACUUM_HOLD → VACUUM_BREAK",
    msgs:[
      { f:"loop",   t:"sm",     txt:"updateStateMachine()"       },
      { f:"sm",     t:"sm",     txt:"changeState(VACUUM_BREAK)", hl:true },
      { f:"sm",     t:"ctrl",   txt:"controlPump(false)"         },
      { f:"sm",     t:"ctrl",   txt:"controlValve(true)"         },
    ]
  },
  { title:"⑥ VACUUM_BREAK → WAIT_REMOVAL",
    msgs:[
      { f:"loop",   t:"sm",     txt:"updateStateMachine()"       },
      { f:"sm",     t:"sm",     txt:"changeState(WAIT_REMOVAL)", hl:true },
      { f:"sm",     t:"ctrl",   txt:"controlValve(false)"        },
      { f:"sm",     t:"sm",     txt:"buzzer ON  200 ms"          },
    ]
  },
  { title:"⑦ WAIT_REMOVAL → COMPLETE",
    msgs:[
      { f:"loop",   t:"sensor", txt:"readSensors()\n→ photoSensor = false" },
      { f:"loop",   t:"sm",     txt:"updateStateMachine()"       },
      { f:"sm",     t:"sm",     txt:"changeState(COMPLETE)",     hl:true },
      { f:"sm",     t:"sm",     txt:"stats.successfulCycles ++"  },
      { f:"sm",     t:"sd",     txt:"logCycle()"                 },
      { f:"sm",     t:"sm",     txt:"buzzer × 2"                 },
    ]
  },
  { title:"⑧ COMPLETE → IDLE",
    msgs:[
      { f:"loop",   t:"sm",     txt:"updateStateMachine()"       },
      { f:"sm",     t:"sm",     txt:"changeState(IDLE)",         hl:true },
      { f:"sm",     t:"ctrl",   txt:"controlPump(false)\ncontrolValve(false)" },
    ]
  },
];

/* ═══════════════════════════════════════════════════════════
   유틸
   ═══════════════════════════════════════════════════════════ */
function norm(dx, dy) {
  const l = Math.hypot(dx, dy) || 1;
  return [dx/l, dy/l];
}

/* ═══════════════════════════════════════════════════════════
   상태 머신 다이어그램
   ═══════════════════════════════════════════════════════════ */
function StateDiagram() {
  const [sel, setSel] = useState(null); // 선택된 상태 id

  const R  = 40;  // 원 반지름
  const W  = 790;
  const H  = 530;

  /* 엣지 경로 생성 */
  function renderEdge(e, idx) {
    const [ax, ay] = XY[e.from] || [0,0];
    const [bx, by] = XY[e.to]   || [0,0];
    const [nx, ny] = norm(bx-ax, by-ay);

    // 원둘레 시작·끝
    const sx = ax + nx*(R+2),  sy = ay + ny*(R+2);
    const ex = bx - nx*(R+8), ey = by - ny*(R+8);

    // 수직 방향으로 곡선 오프셋
    const [px, py] = [-ny, nx];
    const bend = e.err ? 28 : 13;
    const mx = (sx+ex)/2 + px*bend;
    const my = (sy+ey)/2 + py*bend;

    const active = sel===null || sel===e.from || sel===e.to;
    const col    = e.err ? T.red : T.t2;
    const opac   = active ? 1 : 0.15;

    return (
      <g key={`e${idx}`} opacity={opac}>
        <path d={`M${sx},${sy} Q${mx},${my} ${ex},${ey}`}
          fill="none" stroke={col} strokeWidth={1.8}
          strokeDasharray={e.err ? "6 3" : undefined}
          markerEnd={e.err ? "url(#arrRed)" : "url(#arrGray)"} />
        {/* 라벨 */}
        <text x={mx} y={my-10} textAnchor="middle" fontSize={7.6}
          fill={e.err ? T.red : T.t2} fontFamily={MONO} style={{pointerEvents:"none"}}>
          {e.cond.split("\n").map((l,i)=>(
            <tspan key={i} x={mx} dy={i===0?0:11}>{l}</tspan>
          ))}
        </text>
      </g>
    );
  }

  /* 상태 원 */
  function renderNode(id) {
    const [cx, cy] = XY[id];
    const n = NODES[id];
    const isSel  = sel === id;
    const active = sel === null || isSel;

    return (
      <g key={id} opacity={active?1:0.18} style={{cursor:"pointer"}}
        onClick={()=> setSel(isSel ? null : id)}>
        {/* 외발광 (선택시) */}
        {isSel && <circle cx={cx} cy={cy} r={R+10} fill={n.col} opacity={0.1}/>}
        {/* 본체 원 */}
        <circle cx={cx} cy={cy} r={R} fill={T.card} stroke={n.col}
          strokeWidth={isSel?2.6:1.6} />
        {/* 내부 테두리 (얇은 링) */}
        <circle cx={cx} cy={cy} r={R-6} fill="none" stroke={n.col}
          strokeWidth={1} opacity={0.28}/>
        {/* 라벨 */}
        <text x={cx} y={cy-2} textAnchor="middle" fontSize={9.2} fontWeight="700"
          fill={n.col} fontFamily={MONO}>{n.label}</text>
        <text x={cx} y={cy+11} textAnchor="middle" fontSize={7}
          fill={T.t3} fontFamily={MONO}>{n.sub}</text>
      </g>
    );
  }

  /* 선택 시 관련 전이 정보 카드 */
  function InfoCard() {
    if (!sel) return null;
    const inEdges  = EDGES.filter(e => e.to === sel);
    const outEdges = EDGES.filter(e => e.from === sel);
    const globalIn = GLOBALS.filter(g => g.to === sel);
    const n = NODES[sel];
    return (
      <div style={{
        marginTop:12, padding:"10px 14px", borderRadius:6,
        background:T.card, border:`1px solid ${n.col}44`,
        display:"flex", gap:28, flexWrap:"wrap",
      }}>
        <div>
          <div style={{fontSize:8, color:T.t3, fontFamily:MONO, letterSpacing:1.5, textTransform:"uppercase", marginBottom:4}}>진입 (Incoming)</div>
          {globalIn.map((g,i)=>(
            <div key={`g${i}`} style={{fontSize:10, fontFamily:MONO, color:T.red, marginBottom:2}}>
              * (전체) — {g.cond}
            </div>
          ))}
          {inEdges.map((e,i)=>(
            <div key={i} style={{fontSize:10, fontFamily:MONO, color:T.t1, marginBottom:2}}>
              {e.from} — {e.cond.replace(/\n/g," / ")}
            </div>
          ))}
        </div>
        <div>
          <div style={{fontSize:8, color:T.t3, fontFamily:MONO, letterSpacing:1.5, textTransform:"uppercase", marginBottom:4}}>종료 (Outgoing)</div>
          {outEdges.map((e,i)=>(
            <div key={i} style={{fontSize:10, fontFamily:MONO, color:T.t1, marginBottom:2}}>
              {e.cond.replace(/\n/g," / ")} → {e.to}
            </div>
          ))}
        </div>
      </div>
    );
  }

  return (
    <div>
      <svg width="100%" viewBox={`0 0 ${W} ${H}`} style={{display:"block", maxWidth:W}}>
        <defs>
          <marker id="arrGray" markerWidth="8" markerHeight="6" refX="7" refY="3" orient="auto">
            <polygon points="0 0,8 3,0 6" fill={T.t2}/>
          </marker>
          <marker id="arrRed" markerWidth="8" markerHeight="6" refX="7" refY="3" orient="auto">
            <polygon points="0 0,8 3,0 6" fill={T.red}/>
          </marker>
        </defs>

        {/* 엣지 */}
        {EDGES.map((e,i) => renderEdge(e,i))}

        {/* 노드 */}
        {Object.keys(NODES).map(id => renderNode(id))}

        {/* 글로벌 트리거 legends */}
        <g>
          <rect x={14} y={H-68} width={238} height={54} rx={5} fill={T.card} stroke={T.border}/>
          <text x={28} y={H-48} fontSize={7.8} fill={T.t3} fontFamily={MONO}>▸ 글로벌 트리거 (모든 상태에서 즉시 전이)</text>
          <line x1={28} y1={H-33} x2={64} y2={H-33} stroke={T.red} strokeWidth={1.6} strokeDasharray="6 3"/>
          <text x={70} y={H-30} fontSize={7.2} fill={T.red} fontFamily={MONO}>과전류 감지 → ERROR</text>
          <line x1={28} y1={H-18} x2={64} y2={H-18} stroke={T.red} strokeWidth={1.6} strokeDasharray="6 3"/>
          <text x={70} y={H-15} fontSize={7.2} fill={T.red} fontFamily={MONO}>비상정지 SW → EMERGENCY</text>
        </g>
      </svg>
      <InfoCard />
    </div>
  );
}

/* ═══════════════════════════════════════════════════════════
   시퀀스 다이어그════════════════════════════════════════════ */
function SeqDiagram() {
  const [sel, setSel] = useState(null);  // null=전체, 숫자=해당 phase만

  const COL = 106;   // actor 간격
  const PAD = 52;    // 좌우 여백
  const W   = PAD*2 + (ACTORS.length-1)*COL;

  const actorX = (i) => PAD + i*COL;
  const actorIdx = (id) => ACTORS.findIndex(a => a.id === id);

  /* 렌더할 phases 결정 */
  const phases = sel !== null ? [PHASES[sel]] : PHASES;

  /* 행 레이아웃 계산 */
  const HEADER = 50;
  const PH_H   = 28;   // phase 타이틀 행
  const MSG_H  = 28;   // 메시지 행 기본
  const GAP    = 6;    // phase 간 간격

  let rows = [];  // { type, y, h, ... }
  let y    = HEADER;
  phases.forEach((ph, pi) => {
    if (pi > 0) { rows.push({type:"gap", y, h:GAP}); y += GAP; }
    rows.push({type:"phase", y, h:PH_H, title:ph.title});
    y += PH_H;
    ph.msgs.forEach(m => {
      const lines = (m.txt||"").split("\n").length;
      const h = MSG_H + (lines-1)*12;
      rows.push({type:"msg", y, h, msg:m});
      y += h;
    });
  });
  const H = y + 16;

  /* 메시지 렌더 */
  function renderMsg(row, ri) {
    const { msg:m, y:ry, h } = row;
    const fi = actorIdx(m.f), ti = actorIdx(m.t);
    const fx = actorX(fi),    tx = actorX(ti);
    const midY = ry + 15;
    const isSelf = m.f === m.t;
    const isHL   = !!m.hl;
    const col    = isHL ? T.cyan : T.t1;

    if (isSelf) {
      const bx = fx + 38;
      return (
        <g key={`m${ri}`}>
          {isHL && <rect x={0} y={ry} width={W} height={h} fill={T.cyan+"0d"}/>}
          <polyline points={`${fx+3},${midY} ${bx},${midY} ${bx},${midY+13} ${fx+3},${midY+13}`}
            fill="none" stroke={isHL ? T.cyan : T.t3} strokeWidth={isHL?1.7:1}
            markerEnd={isHL ? "url(#sHL)" : "url(#sDim)"} />
          <text x={fx+42} y={midY-1} fontSize={7.6} fill={isHL?col:T.t3} fontFamily={MONO}>{m.txt}</text>
        </g>
      );
    }

    /* 일반 화살표 */
    const goR   = fx < tx;
    const sx    = goR ? fx+4   : fx-4;
    const ex    = goR ? tx-7   : tx+7;
    const labX  = (sx+ex)/2;
    const lines = (m.txt||"").split("\n");

    return (
      <g key={`m${ri}`}>
        {isHL && <rect x={0} y={ry} width={W} height={h} fill={T.cyan+"0d"}/>}
        <line x1={sx} y1={midY} x2={ex} y2={midY}
          stroke={col} strokeWidth={isHL?1.8:1.1}
          markerEnd={isHL ? "url(#sHL)" : "url(#sNorm)"} />
        {lines.map((l,li)=>(
          <text key={li} x={labX} y={midY - 4 - (lines.length-1-li)*12}
            textAnchor="middle" fontSize={7.6} fill={col} fontFamily={MONO}>{l}</text>
        ))}
      </g>
    );
  }

  return (
    <div>
      {/* Phase 필터 버튼 */}
      <div style={{display:"flex", gap:5, flexWrap:"wrap", marginBottom:10}}>
        {[{i:null,label:"전체"}, ...PHASES.map((ph,i)=>({i, label:ph.title.slice(0,2)}))].map(btn => {
          const act = sel === btn.i;
          return (
            <button key={btn.label} onClick={()=>setSel(btn.i === sel ? null : btn.i)} style={{
              padding:"3px 9px", borderRadius:4,
              border:`1px solid ${act ? T.cyan : T.border}`,
              background: act ? T.cyan+"22" : "transparent",
              color: act ? T.cyan : T.t3,
              fontSize:9, fontFamily:MONO, cursor:"pointer",
            }}>{btn.label}</button>
          );
        })}
      </div>

      <div style={{overflowX:"auto", borderRadius:6, border:`1px solid ${T.border}`, background:T.bg}}>
        <svg width={W} height={H} style={{display:"block", minWidth:W}}>
          <defs>
            <marker id="sNorm" markerWidth="7" markerHeight="5" refX="6" refY="2.5" orient="auto">
              <polygon points="0 0,7 2.5,0 5" fill={T.t1}/>
            </marker>
            <marker id="sHL" markerWidth="7" markerHeight="5" refX="6" refY="2.5" orient="auto">
              <polygon points="0 0,7 2.5,0 5" fill={T.cyan}/>
            </marker>
            <marker id="sDim" markerWidth="7" markerHeight="5" refX="6" refY="2.5" orient="auto">
              <polygon points="0 0,7 2.5,0 5" fill={T.t3}/>
            </marker>
          </defs>

          <rect width={W} height={H} fill={T.bg}/>

          {/* Actor 헤더 + 라이프라인 */}
          {ACTORS.map((a,i) => {
            const x = actorX(i);
            return (
              <g key={a.id}>
                <line x1={x} y1={HEADER} x2={x} y2={H-8}
                  stroke={a.col} strokeWidth={1} opacity={0.2} strokeDasharray="3 5"/>
                <rect x={x-36} y={6} width={72} height={24} rx={4}
                  fill={T.card} stroke={a.col} strokeWidth={1.3}/>
                <text x={x} y={22} textAnchor="middle" fontSize={8} fontWeight="600"
                  fill={a.col} fontFamily={MONO}>{a.name}</text>
              </g>
            );
          })}

          {/* 행 렌더링 */}
          {rows.map((row, ri) => {
            if (row.type === "gap") return null;
            if (row.type === "phase") {
              return (
                <g key={`p${ri}`}>
                  <rect x={0} y={row.y} width={W} height={row.h} fill={T.border+"22"}/>
                  <rect x={0} y={row.y} width={3} height={row.h} fill={T.cyan}/>
                  <text x={12} y={row.y+18} fontSize={8.5} fontWeight="700"
                    fill={T.cyan} fontFamily={MONO}>{row.title}</text>
                </g>
              );
            }
            return renderMsg(row, ri);
          })}
        </svg>
      </div>
    </div>
  );
}

/* ═══════════════════════════════════════════════════════════
   Root
   ═══════════════════════════════════════════════════════════ */
export default function App() {
  const [tab, setTab] = useState("state");

  const tabs = [
    { id:"state", label:"상태 머신 다이어그램" },
    { id:"seq",   label:"시퀀스 다이어그램"    },
  ];

  return (
    <div style={{background:T.bg, minHeight:"100vh", color:T.t1, fontFamily:MONO, padding:"20px 14px"}}>

      {/* 타이틀 블록 */}
      <div style={{textAlign:"center", marginBottom:18}}>
        <div style={{fontSize:9.5, color:T.t3, letterSpacing:3, textTransform:"uppercase", marginBottom:3}}>
          ESP32-S3  진공 제어 시스템  v3.3.1
        </div>
        <div style={{fontSize:18, color:T.t1, fontWeight:700, letterSpacing:0.5}}>
          UML 다이어그램
        </div>
      </div>

      {/* 탭 switcher */}
      <div style={{display:"flex", justifyContent:"center", gap:8, marginBottom:16}}>
        {tabs.map(t => {
          const act = tab===t.id;
          return (
            <button key={t.id} onClick={()=>setTab(t.id)} style={{
              padding:"6px 18px", borderRadius:6,
              border:`1px solid ${act ? T.cyan : T.border}`,
              background: act ? T.cyan+"1e" : T.card,
              color: act ? T.cyan : T.t3,
              fontSize:11, fontFamily:MONO, cursor:"pointer",
              boxShadow: act ? `0 0 12px ${T.cyan}30` : "none",
              transition:"all .18s",
            }}>{t.label}</button>
          );
        })}
      </div>

      {/* 본체 카드 */}
      <div style={{
        background:T.card, borderRadius:10, border:`1px solid ${T.border}`,
        padding:"16px 14px", maxWidth:860, margin:"0 auto",
        boxShadow:"0 6px 32px #00000060",
      }}>
        {/* 구분선 + 소제목 */}
        <div style={{display:"flex", alignItems:"center", gap:10, marginBottom:8}}>
          <div style={{flex:1, height:1, background:`linear-gradient(90deg,${T.border},transparent)`}}/>
          <span style={{fontSize:10, letterSpacing:2.2, textTransform:"uppercase", color:T.cyan}}>
            {tab==="state" ? "State Machine — 8 States · 9 Transitions" : "Sequence — Normal Vacuum Cycle"}
          </span>
          <div style={{flex:1, height:1, background:`linear-gradient(270deg,${T.border},transparent)`}}/>
        </div>

        {/* 힌트 */}
        <div style={{fontSize:9, color:T.t3, marginBottom:14, lineHeight:1.6}}>
          {tab==="state"
            ? "상태를 클릭하면 관련 전이가 강조됩니다 · 실선 = 정상 전이 · 빨간 점선 = 에러 / 비상 경로"
            : "밝은 파란색 행 = changeState() 호출 시점 · 상단 버튼으로 구간별 확대 가능"}
        </div>

        {tab==="state" && <StateDiagram />}
        {tab==="seq"   && <SeqDiagram   />}
      </div>
    </div>
  );
}