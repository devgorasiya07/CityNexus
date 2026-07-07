const canvas = document.getElementById('cityCanvas');
const ctx = canvas.getContext('2d');

// Resize canvas to full screen
function resize() {
    canvas.width = window.innerWidth;
    canvas.height = window.innerHeight;
    draw();
}
window.addEventListener('resize', resize);

// UI Elements
const nodeCountVal = document.getElementById('nodeCountVal');
const edgeCountVal = document.getElementById('edgeCountVal');

const statExplored = document.getElementById('statExplored');
const statLength = document.getElementById('statLength');
const statTime = document.getElementById('statTime');

const srcNodeInput = document.getElementById('srcNodeInput');
const tgtNodeInput = document.getElementById('tgtNodeInput');

// Global State
let nodes = [];
let edges = []; // {u, v, weight}
let sourceNode = -1;
let targetNode = -1;
let isAnimating = false;
let stopAnimation = false;

// Drawing Colors
const COLOR_BG = '#0a0a0f';
const COLOR_EDGE = 'rgba(255,255,255,0.05)';
const COLOR_NODE = 'rgba(255,255,255,0.2)';
const COLOR_EXPLORING = 'rgba(0, 242, 254, 0.8)'; // Cyan
const COLOR_VISITED = 'rgba(254, 9, 121, 0.3)';    // Magenta
const COLOR_PATH = '#feca57';                      // Yellow
const COLOR_FACILITY = '#1dd1a1';                  // Green

// ================== GRAPH GENERATION ==================

function generateGrid() {
    if (isAnimating) stopAnimation = true;
    nodes = [];
    edges = [];
    sourceNode = -1;
    targetNode = -1;
    resetStats();

    const count = parseInt(nodeCountVal.value) || 1000;
    if (count > 10000) { alert("Maximum allowed nodes is 10,000 to ensure smooth performance."); return; }
    
    const cols = Math.floor(Math.sqrt(count * (window.innerWidth / window.innerHeight)));
    const rows = Math.floor(count / cols);
    
    // Calculate spacing to fit screen with padding
    const paddingX = 450; // Leave space for dashboard
    const paddingY = 100;
    const spacingX = (window.innerWidth - paddingX - 100) / (cols - 1 || 1);
    const spacingY = (window.innerHeight - paddingY * 2) / (rows - 1 || 1);

    for (let r = 0; r < rows; r++) {
        for (let c = 0; c < cols; c++) {
            nodes.push({
                id: r * cols + c,
                x: paddingX + c * spacingX + (Math.random() * 10 - 5), // Slight organic jitter
                y: paddingY + r * spacingY + (Math.random() * 10 - 5),
                state: 'unvisited' // unvisited, exploring, visited, path, facility
            });
        }
    }

    // Connect neighbors
    for (let r = 0; r < rows; r++) {
        for (let c = 0; c < cols; c++) {
            const u = r * cols + c;
            if (c < cols - 1) edges.push(createEdge(u, u + 1)); // Right
            if (r < rows - 1) edges.push(createEdge(u, u + cols)); // Down
            // Add some diagonals randomly for realism
            if (c < cols - 1 && r < rows - 1 && Math.random() < 0.3) edges.push(createEdge(u, u + cols + 1));
        }
    }
    
    edgeCountVal.value = edges.length;
    draw();
}

function generateRandom() {
    if (isAnimating) stopAnimation = true;
    nodes = [];
    edges = [];
    sourceNode = -1;
    targetNode = -1;
    resetStats();

    const count = parseInt(nodeCountVal.value) || 1000;
    let targetEdges = parseInt(edgeCountVal.value) || 2000;
    const paddingX = 450;
    
    if (count > 10000) {
        alert("Maximum allowed nodes is 10,000 to ensure smooth performance.");
        return;
    }
    
    for (let i = 0; i < count; i++) {
        nodes.push({
            id: i,
            x: paddingX + Math.random() * (window.innerWidth - paddingX - 100),
            y: 100 + Math.random() * (window.innerHeight - 200),
            state: 'unvisited'
        });
    }

    // Step 1: Guarantee a connected graph by building a random Minimum Spanning Tree
    // To keep it visually nice, we connect to nearest neighbor not already in tree
    const inTree = new Set([0]);
    const outTree = new Set();
    for(let i=1; i<count; i++) outTree.add(i);

    while(outTree.size > 0 && edges.length < targetEdges) {
        // Naive closest pair between inTree and outTree
        // Just pick a random outTree node and connect to its closest inTree node
        let arrOut = Array.from(outTree);
        let u = arrOut[Math.floor(Math.random() * arrOut.length)];
        
        let minDist = Infinity;
        let bestV = -1;
        for(let v of inTree) {
            let d = dist(nodes[u], nodes[v]);
            if(d < minDist) { minDist = d; bestV = v; }
        }
        
        edges.push(createEdge(u, bestV));
        inTree.add(u);
        outTree.delete(u);
    }

    // Step 2: Add remaining edges randomly based on proximity
    let attempts = 0;
    while(edges.length < targetEdges && attempts < targetEdges * 3) {
        attempts++;
        let u = Math.floor(Math.random() * count);
        let v = Math.floor(Math.random() * count);
        if (u === v) continue;
        
        // Only connect if somewhat close to keep it looking like a city
        if (dist(nodes[u], nodes[v]) < 250) {
            edges.push(createEdge(u, v));
        }
    }
    
    draw();
}

function createEdge(u, v) {
    // We calculate the physical pixel length of the road
    const baseLength = dist(nodes[u], nodes[v]);
    
    // We assign a random "Traffic Multiplier" to simulate realistic road conditions.
    // Some roads are clear (1.0), some have heavy traffic (up to 3.0).
    // This gives edges different weights, forcing A* and Dijkstra to find creative paths!
    const trafficMultiplier = 1.0 + Math.random() * 2.0; 
    
    return { 
        u, v, 
        weight: baseLength * trafficMultiplier, 
        state: 'unvisited' 
    };
}

function dist(n1, n2) {
    return Math.sqrt(Math.pow(n1.x - n2.x, 2) + Math.pow(n1.y - n2.y, 2));
}

// ================== DRAWING ==================

function draw() {
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    const isMassive = nodes.length > 20000;

    // Fast Batched Rendering for Unvisited Edges
    if (!isMassive) {
        ctx.beginPath();
        edges.forEach(e => {
            if (e.state === 'unvisited') {
                ctx.moveTo(nodes[e.u].x, nodes[e.u].y);
                ctx.lineTo(nodes[e.v].x, nodes[e.v].y);
            }
        });
        ctx.strokeStyle = COLOR_EDGE;
        ctx.lineWidth = 1;
        ctx.shadowBlur = 0;
        ctx.stroke();
    }

    // Draw active edges individually for colors
    edges.forEach(e => {
        if (e.state === 'unvisited') return;
        ctx.beginPath();
        ctx.moveTo(nodes[e.u].x, nodes[e.u].y);
        ctx.lineTo(nodes[e.v].x, nodes[e.v].y);
        
        if (e.state === 'path') {
            ctx.strokeStyle = COLOR_PATH;
            ctx.lineWidth = isMassive ? 1 : 4;
            ctx.shadowBlur = isMassive ? 0 : 10;
            ctx.shadowColor = COLOR_PATH;
        } else if (e.state === 'visited') {
            ctx.strokeStyle = COLOR_VISITED;
            ctx.lineWidth = isMassive ? 1 : 2;
            ctx.shadowBlur = isMassive ? 0 : 8;
            ctx.shadowColor = COLOR_VISITED;
        }
        ctx.stroke();
    });

    // Fast Batched Rendering for Unvisited Nodes
    if (!isMassive) {
        ctx.beginPath();
        nodes.forEach(n => {
            if (n.state === 'unvisited') {
                ctx.rect(n.x - 1, n.y - 1, 2, 2);
            }
        });
        ctx.fillStyle = COLOR_NODE;
        ctx.shadowBlur = 0;
        ctx.fill();
    }

    // Draw active nodes
    nodes.forEach(n => {
        // Do not skip the source and target nodes so they are always drawn!
        if (n.state === 'unvisited' && n.id !== sourceNode && n.id !== targetNode) return;
        
        ctx.beginPath();
        if (n.id === sourceNode || n.id === targetNode) {
            ctx.fillStyle = '#fff';
            ctx.shadowBlur = 10;
            ctx.shadowColor = '#fff';
            ctx.arc(n.x, n.y, 6, 0, Math.PI * 2);
        } else if (n.state === 'facility') {
            ctx.fillStyle = COLOR_FACILITY;
            ctx.shadowBlur = 10;
            ctx.shadowColor = COLOR_FACILITY;
            ctx.arc(n.x, n.y, 8, 0, Math.PI * 2); 
        } else if (n.state === 'path') {
            ctx.fillStyle = COLOR_PATH;
            ctx.shadowBlur = isMassive ? 0 : 10;
            ctx.shadowColor = COLOR_PATH;
            ctx.rect(n.x - 1.5, n.y - 1.5, 3, 3);
        } else if (n.state === 'exploring') {
            ctx.fillStyle = COLOR_EXPLORING;
            ctx.shadowBlur = isMassive ? 0 : 15;
            ctx.shadowColor = COLOR_EXPLORING;
            if (isMassive) ctx.rect(n.x - 1, n.y - 1, 2, 2);
            else ctx.arc(n.x, n.y, 4, 0, Math.PI * 2);
        } else if (n.state === 'visited') {
            ctx.fillStyle = COLOR_VISITED;
            ctx.shadowBlur = isMassive ? 0 : 8;
            ctx.shadowColor = COLOR_VISITED;
            if (isMassive) ctx.rect(n.x - 1, n.y - 1, 2, 2);
            else ctx.arc(n.x, n.y, 3, 0, Math.PI * 2);
        }
        ctx.fill();
    });
    ctx.shadowBlur = 0;
}

function resetGraphState() {
    nodes.forEach(n => { if (n.state !== 'facility') n.state = 'unvisited'; });
    edges.forEach(e => e.state = 'unvisited');
    resetStats();
}

function resetStats() {
    statExplored.innerText = '0';
    statLength.innerText = '0';
    statTime.innerText = '0.00';
}

function updateInputs() {
    srcNodeInput.value = sourceNode !== -1 ? sourceNode : '';
    tgtNodeInput.value = targetNode !== -1 ? targetNode : '';
}

// ================== INTERACTIONS ==================

srcNodeInput.addEventListener('change', (e) => {
    let val = parseInt(e.target.value);
    if (!isNaN(val) && val >= 0 && val < nodes.length) {
        sourceNode = val;
        resetGraphState();
        draw();
    }
});

tgtNodeInput.addEventListener('change', (e) => {
    let val = parseInt(e.target.value);
    if (!isNaN(val) && val >= 0 && val < nodes.length) {
        targetNode = val;
        resetGraphState();
        draw();
    }
});

canvas.addEventListener('click', (e) => {
    if (isAnimating) return;
    const rect = canvas.getBoundingClientRect();
    const x = e.clientX - rect.left;
    const y = e.clientY - rect.top;

    // Find closest node
    let closest = -1;
    let minDist = Infinity;
    nodes.forEach(n => {
        const d = dist({x,y}, n);
        if (d < minDist) { minDist = d; closest = n.id; }
    });

    if (minDist < 20) {
        if (sourceNode === -1) {
            sourceNode = closest;
        } else if (targetNode === -1 && closest !== sourceNode) {
            targetNode = closest;
        } else {
            sourceNode = closest;
            targetNode = -1;
            resetGraphState();
        }
        updateInputs();
        draw();
    }
});

document.getElementById('btnPickRandom').addEventListener('click', () => {
    if (nodes.length === 0 || isAnimating) return;
    resetGraphState();
    sourceNode = Math.floor(Math.random() * nodes.length);
    targetNode = Math.floor(Math.random() * nodes.length);
    while(targetNode === sourceNode) targetNode = Math.floor(Math.random() * nodes.length);
    updateInputs();
    draw();
});

document.getElementById('btnClear').addEventListener('click', () => {
    if (isAnimating) stopAnimation = true;
    nodes.forEach(n => n.state = 'unvisited');
    edges.forEach(e => e.state = 'unvisited');
    sourceNode = -1;
    targetNode = -1;
    resetStats();
    updateInputs();
    draw();
});

// ================== ALGORITHMS ==================

const sleep = ms => new Promise(r => setTimeout(r, ms));

async function runPathfinding(algorithm) {
    if (sourceNode === -1 || targetNode === -1) {
        alert("Please select or randomize a Source and Target node first!");
        return;
    }
    if (isAnimating) return;
    
    resetGraphState();
    isAnimating = true;
    stopAnimation = false;
    
    // Build adjacency list for fast lookup
    const adj = Array.from({length: nodes.length}, () => []);
    edges.forEach((e, idx) => {
        adj[e.u].push({v: e.v, w: e.weight, edgeIdx: idx});
        adj[e.v].push({v: e.u, w: e.weight, edgeIdx: idx});
    });

    // Min-Priority Queue (naive implementation for demo purposes)
    const pq = [];
    const distances = Array(nodes.length).fill(Infinity);
    const parent = Array(nodes.length).fill(null);
    
    distances[sourceNode] = 0;
    pq.push({id: sourceNode, dist: 0, priority: 0});
    
    let exploredCount = 0;
    let computeTime = 0;

    while (pq.length > 0) {
        if (stopAnimation) break;

        let t0 = performance.now();
        
        // Extract min
        pq.sort((a, b) => a.priority - b.priority);
        const u = pq.shift().id;

        if (u === targetNode) break;

        if (nodes[u].state === 'visited') continue;
        
        if (u !== sourceNode) nodes[u].state = 'visited';
        exploredCount++;

        // Visualize exploration frontier
        for (const edge of adj[u]) {
            const v = edge.v;
            const newDist = distances[u] + edge.w;
            
            if (newDist < distances[v]) {
                distances[v] = newDist;
                parent[v] = { u: u, edgeIdx: edge.edgeIdx };
                
                // Heuristic for A*
                let h = 0;
                if (algorithm === 'astar') {
                    h = dist(nodes[v], nodes[targetNode]); // Euclidean distance
                }
                
                pq.push({id: v, dist: newDist, priority: newDist + h});
                
                if (v !== targetNode) nodes[v].state = 'exploring';
                edges[edge.edgeIdx].state = 'visited';
            }
        }
        
        let t1 = performance.now();
        computeTime += (t1 - t0);

        // Update UI every N iterations for smooth animation
        if (exploredCount % 5 === 0) {
            statExplored.innerText = exploredCount;
            draw();
            await sleep(0); // Yield to render
        }
    }

    statTime.innerText = computeTime.toFixed(2);
    statExplored.innerText = exploredCount;

    // Backtrack path
    if (distances[targetNode] !== Infinity && !stopAnimation) {
        let curr = targetNode;
        while (curr !== sourceNode) {
            nodes[curr].state = 'path';
            const p = parent[curr];
            edges[p.edgeIdx].state = 'path';
            curr = p.u;
            draw();
            await sleep(10); // Animate path trace
        }
        nodes[sourceNode].state = 'path';
        statLength.innerText = distances[targetNode].toFixed(0);
        draw();
    } else if (!stopAnimation) {
        alert("No path found!");
    }

    isAnimating = false;
    stopAnimation = false;
}

document.getElementById('btnRunDijkstra').addEventListener('click', () => runPathfinding('dijkstra'));
document.getElementById('btnRunAStar').addEventListener('click', () => runPathfinding('astar'));

document.getElementById('btnPlaceFacilities').addEventListener('click', async () => {
    if (nodes.length === 0 || isAnimating) return;
    resetGraphState();
    
    const k = parseInt(document.getElementById('hospitalCount').value) || 3;
    let facilities = [];
    isAnimating = true;
    stopAnimation = false;
    
    // Pick first hospital randomly
    let first = Math.floor(Math.random() * nodes.length);
    facilities.push(first);
    nodes[first].state = 'facility';
    draw();
    
    // Greedy k-center algorithm (Maximizing the minimum distance)
    // Speed up animation if deploying a massive amount of hospitals
    let delay = 300;
    if (k > 10) delay = 50;
    if (k > 50) delay = 0;

    let computeTime = 0;

    for(let i = 1; i < k; i++) {
        if (stopAnimation) break;
        if (delay > 0) await sleep(delay); 
        
        let t0 = performance.now();
        let maxMinDist = -1;
        let bestNode = -1;
        
        for(let n = 0; n < nodes.length; n++) {
            if(facilities.includes(n)) continue;
            let minDist = Infinity;
            for(let f of facilities) {
                let d = dist(nodes[n], nodes[f]);
                if(d < minDist) minDist = d;
            }
            if(minDist > maxMinDist) {
                maxMinDist = minDist;
                bestNode = n;
            }
        }
        let t1 = performance.now();
        computeTime += (t1 - t0);
        
        facilities.push(bestNode);
        nodes[bestNode].state = 'facility';
        
        // Don't draw every single frame if placing 1000s of facilities
        if (delay > 0 || i % 10 === 0) {
            draw();
        }
    }
    
    statTime.innerText = computeTime.toFixed(2);
    isAnimating = false;
});

document.getElementById('btnIsochrone').addEventListener('click', async () => {
    let inputStart = parseInt(document.getElementById('ambulanceStartNode').value);
    let timeLimit = parseFloat(document.getElementById('ambulanceTimeLimit').value) || 10;
    let speed = parseFloat(document.getElementById('ambulanceSpeed').value) || 60;
    
    if (isNaN(inputStart) || inputStart < 0 || inputStart >= nodes.length) {
        alert("Please enter a valid Start Node ID.");
        return;
    }
    if (isAnimating) return;
    
    // Override the global sourceNode for drawing consistency
    sourceNode = inputStart;
    
    resetGraphState();
    nodes[sourceNode].state = 'facility'; // Mark source as hospital
    
    isAnimating = true;
    stopAnimation = false;
    
    const adj = Array.from({length: nodes.length}, () => []);
    edges.forEach((e, idx) => {
        adj[e.u].push({v: e.v, w: e.weight, edgeIdx: idx});
        adj[e.v].push({v: e.u, w: e.weight, edgeIdx: idx});
    });

    const pq = [];
    const distances = Array(nodes.length).fill(Infinity);
    distances[sourceNode] = 0;
    pq.push({id: sourceNode, dist: 0});
    
    // Calculate max reach limit based on user input
    const maxReach = timeLimit * speed; 
    let exploredCount = 0;
    let computeTime = 0;

    while(pq.length > 0) {
        if (stopAnimation) break;
        
        let t0 = performance.now();
        pq.sort((a,b) => a.dist - b.dist);
        const u = pq.shift().id;
        
        if (nodes[u].state === 'visited') continue;
        if (u !== sourceNode) nodes[u].state = 'visited';
        exploredCount++;

        for(const edge of adj[u]) {
            const v = edge.v;
            const newDist = distances[u] + edge.w;
            
            if (newDist < distances[v] && newDist <= maxReach) {
                distances[v] = newDist;
                pq.push({id: v, dist: newDist});
                if (v !== sourceNode) nodes[v].state = 'exploring';
                edges[edge.edgeIdx].state = 'visited';
            }
        }
        let t1 = performance.now();
        computeTime += (t1 - t0);
        
        if (exploredCount % 3 === 0) {
            statExplored.innerText = exploredCount;
            draw();
            await sleep(0);
        }
    }
    
    statTime.innerText = computeTime.toFixed(2);
    statExplored.innerText = exploredCount;
    statLength.innerText = maxReach.toFixed(0);
    
    isAnimating = false;
    draw();
});

document.getElementById('btnRunMST').addEventListener('click', async () => {
    if (nodes.length === 0 || isAnimating) return;
    resetGraphState();
    
    isAnimating = true;
    stopAnimation = false;
    
    // Kruskal's Algorithm with Union-Find
    let parent = Array(nodes.length).fill(0).map((_, i) => i);
    function find(i) {
        if (parent[i] === i) return i;
        return parent[i] = find(parent[i]);
    }
    function union(i, j) {
        let rootI = find(i);
        let rootJ = find(j);
        if (rootI !== rootJ) {
            parent[rootI] = rootJ;
            return true;
        }
        return false;
    }
    
    // Sort edges by weight
    let tSort0 = performance.now();
    let sortedEdges = edges.map((e, idx) => ({...e, idx})).sort((a,b) => a.weight - b.weight);
    let computeTime = performance.now() - tSort0;
    
    let mstEdgesCount = 0;
    let totalWeight = 0;
    
    // Speed up animation if graph is huge
    let delay = 50;
    if (nodes.length > 500) delay = 5;
    if (nodes.length > 2000) delay = 0;

    for (let e of sortedEdges) {
        if (stopAnimation) break;
        
        let t0 = performance.now();
        let success = union(e.u, e.v);
        let t1 = performance.now();
        computeTime += (t1 - t0);
        
        if (success) {
            edges[e.idx].state = 'path';
            nodes[e.u].state = 'path';
            nodes[e.v].state = 'path';
            mstEdgesCount++;
            totalWeight += e.weight;
            
            if (delay > 0 || mstEdgesCount % 20 === 0) {
                statExplored.innerText = mstEdgesCount;
                draw();
                await sleep(delay);
            }
        }
        
        if (mstEdgesCount === nodes.length - 1) break;
    }
    
    statTime.innerText = computeTime.toFixed(2);
    statExplored.innerText = mstEdgesCount + " Edges";
    statLength.innerText = totalWeight.toFixed(0);
    
    isAnimating = false;
    draw();
});

// Initialize
resize();
generateGrid();

document.getElementById('btnGenerateGrid').addEventListener('click', generateGrid);
document.getElementById('btnGenerateRandom').addEventListener('click', generateRandom);
