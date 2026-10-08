import { adjacency } from './graph';

// Same algorithm as core-engine/src/Dijkstra.cpp: min-heap-style expansion
// (here via a simple sorted scan, since our graph has only 10 nodes — a real
// binary heap isn't needed for authenticity at this scale, only for the
// asymptotic complexity the C++ version demonstrates).
export function shortestPath(source, destination) {
  const dist = { [source]: 0 };
  const prev = {};
  const visited = new Set();

  while (true) {
    let current = null;
    let currentDist = Infinity;
    for (const node in dist) {
      if (!visited.has(node) && dist[node] < currentDist) {
        current = node;
        currentDist = dist[node];
      }
    }
    if (current === null) break;
    visited.add(current);
    if (Number(current) === destination) break;

    const neighbors = adjacency[current] || [];
    for (const { to, weight } of neighbors) {
      const newDist = currentDist + weight;
      if (dist[to] === undefined || newDist < dist[to]) {
        dist[to] = newDist;
        prev[to] = Number(current);
      }
    }
  }

  if (dist[destination] === undefined) {
    return { reachable: false, distance: -1, path: [] };
  }

  const path = [destination];
  let node = destination;
  while (node !== source) {
    node = prev[node];
    path.unshift(node);
  }

  return { reachable: true, distance: dist[destination], path };
}
