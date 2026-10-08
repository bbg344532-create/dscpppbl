// Mirrors the exact road network defined in core-engine/src/main.cpp (Scenario 1),
// including the real edge weights — so distances computed here match what the
// C++ engine would compute for the same graph, not arbitrary pixel distances.

export const nodePositions = {
  1: { x: 60, y: 260 },
  2: { x: 180, y: 140 },
  3: { x: 180, y: 340 },
  4: { x: 320, y: 140 },
  5: { x: 320, y: 340 },
  6: { x: 460, y: 200 },
  7: { x: 460, y: 340 },
  8: { x: 600, y: 140 },
  9: { x: 600, y: 400 },
  10: { x: 740, y: 260 },
};

// [from, to, weight] — bidirectional, same as graph.addEdge(a, b, w) in main.cpp
const rawEdges = [
  [1, 2, 4], [1, 3, 2], [2, 4, 5], [3, 4, 1], [3, 5, 7],
  [4, 6, 3], [5, 6, 2], [5, 7, 4], [6, 8, 6], [7, 8, 1],
  [7, 9, 5], [8, 10, 2], [9, 10, 3],
];

export const edgesForDisplay = rawEdges.map(([a, b]) => [a, b]);

function buildAdjacency() {
  const adj = {};
  for (const [a, b, w] of rawEdges) {
    if (!adj[a]) adj[a] = [];
    if (!adj[b]) adj[b] = [];
    adj[a].push({ to: b, weight: w });
    adj[b].push({ to: a, weight: w });
  }
  return adj;
}

export const adjacency = buildAdjacency();
