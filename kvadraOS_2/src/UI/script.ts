type CpuMetrics = {
  totalUsage: number;
  loadAvg1: number;
};

type MemMetrics = {
  used: number;
  total: number;
};

type DiskMetrics = Array<Record<string, unknown>>;

type MetricsResponse = {
  timestamp?: number;
  cpu?: Partial<CpuMetrics>;
  mem?: Partial<MemMetrics>;
  disks?: DiskMetrics;
  processes?: ProcessMetrics[];
};

type ProcessMetrics = {
  pid: number;
  nice: number;
  command: string;
  threads: number;
  uid: number;
  memPercent: number;
  cpuPercent: number;
  rssBytes: number;
};

const MAX_POINTS = 120;
type SeriesDef = {
  id: string;
  label: string;
  color: string;
  value: (m: MetricsResponse) => number;
  info: (m: MetricsResponse, v: number) => string;
  format: (v: number) => string;
};

const histories = new Map<string, number[]>();
let activeSeriesId = 'cpu.total';
let lastSnapshot: MetricsResponse | null = null;
let currentProcesses: ProcessMetrics[] = [];

type ProcSortKey = 'cpuPercent' | 'memPercent' | 'rssBytes' | 'pid' | 'threads' | 'nice';
let procSortKey: ProcSortKey = 'cpuPercent';
let procSortDesc = true;

function formatBytes(value: number): string {
  const units = ['B', 'KB', 'MB', 'GB', 'TB', 'PB'];
  let v = Math.max(0, value);
  let i = 0;

  while (v >= 1024 && i < units.length - 1) {
    v /= 1024;
    i++;
  }

  return `${v.toFixed(v >= 10 ? 1 : 2)} ${units[i]}`;
}

function pushPoint(id: string, value: number): void {
  const arr = histories.get(id) || [];
  arr.push(value);
  if (arr.length > MAX_POINTS) {
    arr.splice(0, arr.length - MAX_POINTS);
  }
  histories.set(id, arr);
}

function drawLineChart(values: number[], maxValue: number, color: string): void {
  const canvas = document.getElementById('detailChart') as HTMLCanvasElement | null;
  if (!canvas) {
    return;
  }
  const ctx = canvas.getContext('2d');
  if (!ctx) {
    return;
  }

  const w = canvas.width;
  const h = canvas.height;
  ctx.clearRect(0, 0, w, h);

  ctx.strokeStyle = '#374151';
  ctx.lineWidth = 1;
  ctx.beginPath();
  ctx.moveTo(0, 2);
  ctx.lineTo(w, 2);
  ctx.moveTo(0, h / 2);
  ctx.lineTo(w, h / 2);
  ctx.moveTo(0, h - 1);
  ctx.lineTo(w, h - 1);
  ctx.stroke();

  const topVal = maxValue;
  const midVal = maxValue / 2;
  const lowVal = 0;
  ctx.fillStyle = '#9ca3af';
  ctx.font = '11px sans-serif';
  ctx.fillText(`${topVal.toFixed(0)}`, w - 36, 12);
  ctx.fillText(`${midVal.toFixed(0)}`, w - 36, h / 2 - 4);
  ctx.fillText(`${lowVal.toFixed(0)}`, w - 30, h - 6);

  if (values.length < 2) {
    return;
  }

  ctx.strokeStyle = color;
  ctx.lineWidth = 2;
  ctx.beginPath();

  for (let i = 0; i < values.length; i++) {
    const x = (i / Math.max(1, MAX_POINTS - 1)) * w;
    const norm = Math.max(0, Math.min(1, values[i] / maxValue));
    const y = h - norm * (h - 6) - 3;
    if (i === 0) {
      ctx.moveTo(x, y);
    } else {
      ctx.lineTo(x, y);
    }
  }
  ctx.stroke();

  const last = values[values.length - 1];
  const min = Math.min(...values);
  const max = Math.max(...values);

  ctx.fillStyle = '#9ca3af';
  ctx.font = '12px sans-serif';
  ctx.fillText(`min: ${min.toFixed(2)}`, 8, 16);
  ctx.fillText(`max: ${max.toFixed(2)}`, 120, 16);
  ctx.fillText(`last: ${last.toFixed(2)}`, 232, 16);
}

function buildSeriesList(m: MetricsResponse): SeriesDef[] {
  const list: SeriesDef[] = [
    {
      id: 'cpu.total',
      label: 'CPU',
      color: '#22d3ee',
      value: (x) => Number(x.cpu?.totalUsage || 0),
      info: (x, v) => `CPU total: ${v.toFixed(2)}%\nload1: ${x.cpu?.loadAvg1 ?? 0}`,
      format: (v) => `${v.toFixed(2)}%`
    },
    {
      id: 'mem.used_pct',
      label: 'MEM',
      color: '#a78bfa',
      value: (x) => {
        const used = Number(x.mem?.used || 0);
        const total = Number(x.mem?.total || 0);
        return total > 0 ? (used / total) * 100 : 0;
      },
      info: (x, v) => `MEM used: ${formatBytes(Number(x.mem?.used || 0))}\nMEM total: ${formatBytes(Number(x.mem?.total || 0))}\nUsed: ${v.toFixed(2)}%`,
      format: (v) => `${v.toFixed(2)}%`
    }
  ];

  const disks = m.disks || [];
  for (let i = 0; i < disks.length; i++) {
    const d = disks[i] as Record<string, unknown>;
    const mount = String(d.mountPoint || `disk${i}`);

    list.push({
      id: `disk.${mount}.used_pct`,
      label: `DISK ${mount}`,
      color: '#34d399',
      value: (x) => {
        const disk = (x.disks || [])[i] as Record<string, unknown> | undefined;
        const used = Number(disk?.used || 0);
        const total = Number(disk?.total || 0);
        return total > 0 ? (used / total) * 100 : 0;
      },
      info: (x, v) => {
        const disk = (x.disks || [])[i] as Record<string, unknown> | undefined;
        return [
          `DISK mount: ${String(disk?.mountPoint || mount)}`,
          `Used: ${formatBytes(Number(disk?.used || 0))}`,
          `Total: ${formatBytes(Number(disk?.total || 0))}`,
          `Used %: ${v.toFixed(2)}`,
          `Read: ${formatBytes(Number(disk?.readBytesPerSec || 0))}/s`,
          `Write: ${formatBytes(Number(disk?.writeBytesPerSec || 0))}/s`
        ].join('\n');
      },
      format: (v) => `${v.toFixed(2)}%`
    });
  }

  return list;
}

function renderSeriesMenu(series: SeriesDef[]): void {
  const root = document.getElementById('seriesList');
  if (!root) {
    return;
  }
  root.innerHTML = '';

  for (const s of series) {
    const btn = document.createElement('button');
    btn.className = `series-btn${s.id === activeSeriesId ? ' active' : ''}`;
    const values = histories.get(s.id) || [];
    const current = values.length ? values[values.length - 1] : 0;
    btn.textContent = `${s.label} — ${s.format(current)}`;
    btn.onclick = () => {
      activeSeriesId = s.id;
      renderSeriesMenu(series);
      renderDetail(series);
    };
    root.appendChild(btn);
  }
}

function renderDetail(series: SeriesDef[]): void {
  const active = series.find((x) => x.id === activeSeriesId) || series[0];
  if (!active) {
    return;
  }

  const title = document.getElementById('detailTitle');
  const info = document.getElementById('detailInfo');
  if (title) {
    title.textContent = active.label;
  }

  const values = histories.get(active.id) || [];
  drawLineChart(values, 100, active.color);

  if (info && lastSnapshot) {
    const current = active.value(lastSnapshot);
    info.textContent = active.info(lastSnapshot, current);
  }
}

function applySnapshot(m: MetricsResponse): void {
  lastSnapshot = m;
  currentProcesses = (m.processes || []).slice();
  const series = buildSeriesList(m);

  for (const s of series) {
    pushPoint(s.id, s.value(m));
  }

  if (!series.some((x) => x.id === activeSeriesId)) {
    activeSeriesId = series[0]?.id || activeSeriesId;
  }

  renderSeriesMenu(series);
  renderDetail(series);
  renderProcesses();
}

function renderProcesses(): void {
  const tbody = document.getElementById('processes');
  if (!tbody) {
    return;
  }

  const k1 = procSortKey;
  const desc = procSortDesc;
  const k2: ProcSortKey = k1 === 'cpuPercent' ? 'memPercent' : 'cpuPercent';

  const rows = currentProcesses.slice().sort((a, b) => {
    const cmp = (x: number, y: number) => (x === y ? 0 : x < y ? -1 : 1);
    const c1 = cmp(Number((a as Record<string, unknown>)[k1] || 0), Number((b as Record<string, unknown>)[k1] || 0));
    if (c1 !== 0) {
      return desc ? -c1 : c1;
    }
    const c2 = cmp(Number((a as Record<string, unknown>)[k2] || 0), Number((b as Record<string, unknown>)[k2] || 0));
    return desc ? -c2 : c2;
  });

  tbody.innerHTML = '';
  for (const p of rows) {
    const tr = document.createElement('tr');

    const tdPid = document.createElement('td'); tdPid.textContent = String(p.pid);
    const tdNi = document.createElement('td'); tdNi.textContent = String(p.nice);
    const tdUid = document.createElement('td'); tdUid.textContent = String(p.uid);
    const tdThr = document.createElement('td'); tdThr.textContent = String(p.threads);
    const tdCpu = document.createElement('td'); tdCpu.textContent = p.cpuPercent.toFixed(1);
    const tdMem = document.createElement('td'); tdMem.textContent = p.memPercent.toFixed(1);
    const tdRss = document.createElement('td'); tdRss.textContent = formatBytes(p.rssBytes);
    const tdCmd = document.createElement('td');
    tdCmd.className = 'command';
    tdCmd.textContent = p.command;
    tdCmd.title = p.command;

    tr.append(tdPid, tdNi, tdUid, tdThr, tdCpu, tdMem, tdRss, tdCmd);
    tbody.appendChild(tr);
  }
}

async function loadHistory(): Promise<void> {
  const r = await fetch('/api/history');
  const history = (await r.json()) as MetricsResponse[];

  for (const snap of history) {
    applySnapshot(snap);
  }
}

async function tick(): Promise<void> {
  const r = await fetch('/api/metrics');
  const m = (await r.json()) as MetricsResponse;
  applySnapshot(m);
}

async function start(): Promise<void> {
  const tabMetrics = document.getElementById('tabMetrics') as HTMLButtonElement | null;
  const tabProcesses = document.getElementById('tabProcesses') as HTMLButtonElement | null;
  const viewMetrics = document.getElementById('viewMetrics');
  const viewProcesses = document.getElementById('viewProcesses');

  if (tabMetrics && tabProcesses && viewMetrics && viewProcesses) {
    tabMetrics.onclick = () => {
      tabMetrics.classList.add('active');
      tabProcesses.classList.remove('active');
      viewMetrics.classList.remove('hidden');
      viewProcesses.classList.add('hidden');
    };
    tabProcesses.onclick = () => {
      tabProcesses.classList.add('active');
      tabMetrics.classList.remove('active');
      viewProcesses.classList.remove('hidden');
      viewMetrics.classList.add('hidden');
    };
  }

  document.querySelectorAll<HTMLTableCellElement>('th[data-sort]').forEach((th) => {
    th.addEventListener('click', () => {
      const key = th.dataset.sort as ProcSortKey | undefined;
      if (!key) {
        return;
      }
      if (procSortKey === key) {
        procSortDesc = !procSortDesc;
      } else {
        procSortKey = key;
        procSortDesc = true;
      }
      renderProcesses();
    });
  });

  const splitter = document.getElementById('splitter');
  if (splitter && viewMetrics) {
    let dragging = false;

    splitter.addEventListener('mousedown', () => {
      dragging = true;
    });

    window.addEventListener('mouseup', () => {
      dragging = false;
    });

    window.addEventListener('mousemove', (e) => {
      if (!dragging) {
        return;
      }
      const width = window.innerWidth;
      const minPx = 220;
      const maxPx = Math.max(minPx, Math.floor(width * 0.5));
      const leftPx = Math.min(maxPx, Math.max(minPx, e.clientX));
      viewMetrics.style.gridTemplateColumns = `${leftPx}px 8px 1fr`;
    });
  }

  await loadHistory();
  await tick();
  setInterval(tick, 1000);
}

void start();