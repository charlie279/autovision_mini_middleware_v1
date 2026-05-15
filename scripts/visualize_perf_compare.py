#!/usr/bin/env python3
"""Generate CSV + HTML/SVG summary for RGB vs YUYV preprocess benchmark logs.

Input accepts two formats:
1) preprocess_node --perf-log CSV:
   frame_id,input_format,input_bytes,stride_bytes,preprocess_ms,total_ms,...
2) examples/13_preprocess_benchmark_compare CSV:
   case,input_format,input_bytes,stride_bytes,loop,preprocess_ms,tensor_mean
"""
import argparse
import csv
import html
import statistics
from pathlib import Path


def percentile(values, p):
    if not values:
        return 0.0
    xs = sorted(values)
    k = (len(xs) - 1) * p / 100.0
    lo = int(k)
    hi = min(lo + 1, len(xs) - 1)
    frac = k - lo
    return xs[lo] * (1.0 - frac) + xs[hi] * frac


def load_rows(path: Path, default_case: str):
    rows = []
    with path.open(newline='', encoding='utf-8') as f:
        reader = csv.DictReader(f)
        for row in reader:
            try:
                fmt = row.get('input_format') or default_case
                case = row.get('case') or default_case or fmt
                rows.append({
                    'case': case,
                    'input_format': fmt,
                    'input_bytes': int(float(row.get('input_bytes', '0'))),
                    'stride_bytes': int(float(row.get('stride_bytes', '0'))),
                    'preprocess_ms': float(row.get('preprocess_ms', '0')),
                    'total_ms': float(row.get('total_ms', row.get('preprocess_ms', '0'))),
                })
            except (TypeError, ValueError):
                continue
    return rows


def summarize(rows):
    groups = {}
    for row in rows:
        key = row['case']
        groups.setdefault(key, []).append(row)
    summary = []
    for key, vals in groups.items():
        pre = [v['preprocess_ms'] for v in vals]
        tot = [v['total_ms'] for v in vals]
        bytes_list = [v['input_bytes'] for v in vals if v['input_bytes'] > 0]
        stride_list = [v['stride_bytes'] for v in vals if v['stride_bytes'] > 0]
        summary.append({
            'case': key,
            'input_format': vals[0]['input_format'],
            'frames': len(vals),
            'input_bytes': round(statistics.mean(bytes_list)) if bytes_list else 0,
            'stride_bytes': round(statistics.mean(stride_list)) if stride_list else 0,
            'preprocess_mean_ms': statistics.mean(pre) if pre else 0.0,
            'preprocess_p95_ms': percentile(pre, 95.0),
            'total_mean_ms': statistics.mean(tot) if tot else 0.0,
            'total_p95_ms': percentile(tot, 95.0),
        })
    return sorted(summary, key=lambda x: x['case'])


def write_summary_csv(summary, path: Path):
    path.parent.mkdir(parents=True, exist_ok=True)
    fields = ['case', 'input_format', 'frames', 'input_bytes', 'stride_bytes',
              'preprocess_mean_ms', 'preprocess_p95_ms', 'total_mean_ms', 'total_p95_ms']
    with path.open('w', newline='', encoding='utf-8') as f:
        writer = csv.DictWriter(f, fieldnames=fields)
        writer.writeheader()
        for row in summary:
            writer.writerow(row)


def bar_svg(summary, metric):
    width = 760
    row_h = 42
    height = max(180, 80 + row_h * len(summary))
    max_v = max([row[metric] for row in summary] + [1.0])
    parts = [f'<svg viewBox="0 0 {width} {height}" width="100%" height="{height}" role="img" aria-label="{metric} bar chart">']
    parts.append('<rect x="0" y="0" width="100%" height="100%" fill="white"/>')
    parts.append(f'<text x="20" y="32" font-size="18" font-weight="700">{html.escape(metric)}</text>')
    x0 = 180
    max_w = 500
    for i, row in enumerate(summary):
        y = 60 + i * row_h
        v = row[metric]
        bw = 0 if max_v == 0 else max(1, v / max_v * max_w)
        label = f"{row['case']} ({row['input_format']})"
        parts.append(f'<text x="20" y="{y + 22}" font-size="13">{html.escape(label)}</text>')
        parts.append(f'<rect x="{x0}" y="{y}" width="{bw:.1f}" height="24" rx="4"/>')
        parts.append(f'<text x="{x0 + bw + 8:.1f}" y="{y + 18}" font-size="13">{v:.3f} ms</text>')
    parts.append('</svg>')
    return '\n'.join(parts)


def write_html(summary, path: Path):
    path.parent.mkdir(parents=True, exist_ok=True)
    if not summary:
        body = '<p>No valid benchmark rows were found.</p>'
    else:
        rows = []
        for r in summary:
            rows.append('<tr>' + ''.join([
                f'<td>{html.escape(str(r["case"]))}</td>',
                f'<td>{html.escape(str(r["input_format"]))}</td>',
                f'<td>{r["frames"]}</td>',
                f'<td>{r["input_bytes"]}</td>',
                f'<td>{r["stride_bytes"]}</td>',
                f'<td>{r["preprocess_mean_ms"]:.3f}</td>',
                f'<td>{r["preprocess_p95_ms"]:.3f}</td>',
                f'<td>{r["total_mean_ms"]:.3f}</td>',
                f'<td>{r["total_p95_ms"]:.3f}</td>',
            ]) + '</tr>')
        body = f'''
        <section class="card">{bar_svg(summary, 'preprocess_mean_ms')}</section>
        <section class="card">{bar_svg(summary, 'preprocess_p95_ms')}</section>
        <section class="card">
        <table>
          <thead><tr><th>case</th><th>format</th><th>frames</th><th>input bytes</th><th>stride</th><th>pre mean ms</th><th>pre p95 ms</th><th>total mean ms</th><th>total p95 ms</th></tr></thead>
          <tbody>{''.join(rows)}</tbody>
        </table>
        </section>'''
    path.write_text(f'''<!doctype html>
<html lang="zh-CN">
<head>
<meta charset="utf-8"/>
<title>AutoVision V1.8 Preprocess Performance Compare</title>
<style>
body {{ font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif; margin: 24px; color: #111; }}
.card {{ border: 1px solid #ddd; border-radius: 12px; padding: 16px; margin: 16px 0; box-shadow: 0 1px 4px rgba(0,0,0,.06); }}
table {{ border-collapse: collapse; width: 100%; font-size: 14px; }}
th, td {{ border-bottom: 1px solid #eee; text-align: left; padding: 8px; }}
th {{ background: #f7f7f7; }}
svg rect:not(:first-child) {{ fill: #333; }}
.note {{ color: #555; font-size: 14px; }}
</style>
</head>
<body>
<h1>AutoVision V1.8 Preprocess Performance Compare</h1>
<p class="note">This report is generated from real benchmark CSV files. It does not invent camera results. V1.8 adds index-plan preprocess cases and buffer-lease validation logs.</p>
{body}
</body>
</html>
''', encoding='utf-8')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--input', action='append', required=True, help='CSV log path. Can be passed multiple times.')
    parser.add_argument('--case', action='append', default=[], help='Case name for corresponding --input when CSV has no case column.')
    parser.add_argument('--out-csv', default='logs/benchmark/perf_compare_summary.csv')
    parser.add_argument('--out-html', default='logs/benchmark/perf_compare.html')
    args = parser.parse_args()

    all_rows = []
    for i, item in enumerate(args.input):
        default_case = args.case[i] if i < len(args.case) else Path(item).stem
        all_rows.extend(load_rows(Path(item), default_case))
    summary = summarize(all_rows)
    write_summary_csv(summary, Path(args.out_csv))
    write_html(summary, Path(args.out_html))
    print(f'[visualize_perf_compare] rows={len(all_rows)} groups={len(summary)} out_csv={args.out_csv} out_html={args.out_html}')


if __name__ == '__main__':
    main()
