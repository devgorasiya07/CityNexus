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

// Drawing Colors (synced with CSS design system)
const COLOR_BG = '#0b0d14';
const COLOR_EDGE = 'rgba(255,255,255,0.06)';
const COLOR_NODE = 'rgba(255, 255, 255, 0.15)';
const COLOR_FACILITY = '#00e88f';
const COLOR_PATH = '#ffc940';
const COLOR_EXPLORING = '#fff';
const SIDEBAR_WIDTH = 360; // Match CSS sidebar width + gap

// Global arrays for heatmap visuals
let nodeDistances = [];
let globalMaxDist = 1;

// ================== GRAPH GENERATION ==================


function generateRandom() {
    if (isAnimating) stopAnimation = true;
    nodes = [];
    edges = [];
    sourceNode = -1;
    targetNode = -1;
    resetStats();

    const count = parseInt(nodeCountVal.value) || 1000;
    let targetEdges = parseInt(edgeCountVal.value) || 2000;
    const paddingX = SIDEBAR_WIDTH;
    
    if (count > 10000) {
        alert("Maximum allowed nodes is 10,000 to ensure smooth performance.");
        return;
    }
    
    const maxEdges = Math.floor((count * (count - 1)) / 2);
    if (targetEdges > maxEdges) {
        alert(`Invalid Graph: A city with ${count} intersections can only have a maximum of ${maxEdges} unique roads. Please lower your road count.`);
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
    // Weight = integer road cost = distance × traffic factor
    // Traffic factor (1 to 3) simulates real-world road congestion levels
    // This ensures longer roads cost more, AND that parallel roads can 
    // have different costs — forcing A* to find creative detours
    const length = dist(nodes[u], nodes[v]);
    const trafficFactor = 1 + Math.floor(Math.random() * 3); // 1, 2, or 3
    
    return { 
        u, v, 
        weight: Math.round(length * trafficFactor), 
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
    ctx.beginPath();
    ctx.strokeStyle = COLOR_EDGE;
    ctx.lineWidth = 1;
    ctx.shadowBlur = 0;
    edges.forEach(e => {
        if (e.state === 'unvisited') {
            ctx.moveTo(nodes[e.u].x, nodes[e.u].y);
            ctx.lineTo(nodes[e.v].x, nodes[e.v].y);
        }
    });
    ctx.stroke();

    // Draw active edges individually for colors
    edges.forEach(e => {
        if (e.state === 'unvisited') return;
        ctx.beginPath();
        ctx.moveTo(nodes[e.u].x, nodes[e.u].y);
        ctx.lineTo(nodes[e.v].x, nodes[e.v].y);
        
        if (e.state === 'path') {
            // Thick gold line with glow
            ctx.strokeStyle = COLOR_PATH;
            ctx.lineWidth = isMassive ? 2 : 5;
            ctx.shadowBlur = isMassive ? 0 : 15;
            ctx.shadowColor = COLOR_PATH;
            ctx.stroke();
            
            // Inner white core for realism
            if (!isMassive) {
                ctx.beginPath();
                ctx.moveTo(nodes[e.u].x, nodes[e.u].y);
                ctx.lineTo(nodes[e.v].x, nodes[e.v].y);
                ctx.strokeStyle = '#fff';
                ctx.lineWidth = 2;
                ctx.shadowBlur = 0;
                ctx.stroke();
            }
            return;
        } else if (e.state === 'visited') {
            // Dynamic Heatmap Gradient based on distance
            let dist = nodeDistances[e.v] || 0;
            let hue = 200 + (dist / (globalMaxDist || 1)) * 140; // Cyan (200) to Pink (340)
            ctx.strokeStyle = `hsl(${hue}, 100%, 55%)`;
            ctx.shadowColor = `hsl(${hue}, 100%, 55%)`;
            ctx.lineWidth = isMassive ? 1 : 2;
            ctx.shadowBlur = isMassive ? 0 : 8;
            ctx.stroke();
        }
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
        } else if (n.state === 'waypoint') {
            ctx.fillStyle = '#00d4ff';
            ctx.shadowBlur = 12;
            ctx.shadowColor = '#00d4ff';
            ctx.arc(n.x, n.y, 7, 0, Math.PI * 2);
        } else if (n.state === 'path') {
            ctx.fillStyle = COLOR_PATH;
            ctx.shadowBlur = isMassive ? 0 : 10;
            ctx.shadowColor = COLOR_PATH;
            ctx.rect(n.x - 1.5, n.y - 1.5, 3, 3);
        } else if (n.state === 'exploring') {
            // Bright white frontier with massive cyan glow
            ctx.fillStyle = '#fff';
            ctx.shadowBlur = isMassive ? 0 : 20;
            ctx.shadowColor = '#00d4ff';
            if (isMassive) ctx.rect(n.x - 1, n.y - 1, 2, 2);
            else ctx.arc(n.x, n.y, 5, 0, Math.PI * 2);
        } else if (n.state === 'visited') {
            // Dynamic Heatmap Gradient
            let dist = nodeDistances[n.id] || 0;
            let hue = 200 + (dist / (globalMaxDist || 1)) * 140;
            ctx.fillStyle = `hsl(${hue}, 100%, 60%)`;
            ctx.shadowBlur = isMassive ? 0 : 10;
            ctx.shadowColor = `hsl(${hue}, 100%, 60%)`;
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
    statExplored.innerText = '—';
    statLength.innerText = '—';
    statTime.innerText = '—';
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
    // Initialize data structures
    const distances = Array(nodes.length).fill(Infinity);
    const parent = Array(nodes.length).fill(null);
    const pq = [];
    
    nodeDistances = distances;
    globalMaxDist = 1;

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
                if (newDist > globalMaxDist && newDist !== Infinity) globalMaxDist = newDist;
                
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
    
    nodeDistances = distances;
    globalMaxDist = 1;
    
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
                if (newDist > globalMaxDist && newDist !== Infinity) globalMaxDist = newDist;
                
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
generateRandom();

document.getElementById('btnGenerateRandom').addEventListener('click', generateRandom);

// ================== MANUAL GRAPH INPUT ==================
document.getElementById('btnLoadManual').addEventListener('click', () => {
    if (isAnimating) stopAnimation = true;
    const input = document.getElementById('manualEdgeInput').value.trim();
    if (!input) { alert('Please enter at least one edge in format: u v w'); return; }

    const lines = input.split('\n').map(l => l.trim()).filter(l => l.length > 0);
    const parsedEdges = [];
    let maxNodeId = -1;

    for (let i = 0; i < lines.length; i++) {
        const parts = lines[i].split(/[,\s]+/).map(Number);
        if (parts.length < 3 || parts.some(isNaN)) {
            alert(`Line ${i + 1} is invalid: "${lines[i]}"\nExpected format: u v w (e.g. 0 1 50)`);
            return;
        }
        const [u, v, w] = parts;
        if (u < 0 || v < 0 || w <= 0) {
            alert(`Line ${i + 1}: Node IDs must be ≥ 0 and weight must be > 0.`);
            return;
        }
        parsedEdges.push({ u, v, w: Math.round(w) });
        maxNodeId = Math.max(maxNodeId, u, v);
    }

    // Create nodes (auto-detect count from max ID)
    nodes = [];
    edges = [];
    sourceNode = -1;
    targetNode = -1;
    resetStats();

    const nodeCount = maxNodeId + 1;
    const paddingX = SIDEBAR_WIDTH + 60;
    const paddingY = 80;
    const areaW = window.innerWidth - paddingX - 60;
    const areaH = window.innerHeight - paddingY * 2;

    // Position nodes in a circle layout for clean visualization
    for (let i = 0; i < nodeCount; i++) {
        const angle = (2 * Math.PI * i) / nodeCount - Math.PI / 2;
        const radius = Math.min(areaW, areaH) / 2.5;
        const cx = paddingX + areaW / 2;
        const cy = paddingY + areaH / 2;
        nodes.push({
            id: i,
            x: cx + radius * Math.cos(angle),
            y: cy + radius * Math.sin(angle),
            state: 'unvisited'
        });
    }

    // Add parsed edges
    parsedEdges.forEach(e => {
        edges.push({ u: e.u, v: e.v, weight: e.w, state: 'unvisited' });
    });

    draw();
});

// ================== FEATURE: ALGORITHM COMPARISON ==================

function runPathfindingSilent(algorithm) {
    // Runs pathfinding WITHOUT animation, returns stats only
    const adj = Array.from({length: nodes.length}, () => []);
    edges.forEach((e, idx) => {
        adj[e.u].push({v: e.v, w: e.weight, edgeIdx: idx});
        adj[e.v].push({v: e.u, w: e.weight, edgeIdx: idx});
    });

    const distances = Array(nodes.length).fill(Infinity);
    const parent = Array(nodes.length).fill(null);
    const pq = [];

    distances[sourceNode] = 0;
    pq.push({id: sourceNode, dist: 0, priority: 0});

    let exploredCount = 0;
    const t0 = performance.now();

    while (pq.length > 0) {
        pq.sort((a, b) => a.priority - b.priority);
        const u = pq.shift().id;

        if (u === targetNode) break;
        if (distances[u] > (distances[u] || Infinity) + 0.001) continue; // stale entry check
        
        exploredCount++;

        for (const edge of adj[u]) {
            const v = edge.v;
            const newDist = distances[u] + edge.w;
            
            if (newDist < distances[v]) {
                distances[v] = newDist;
                parent[v] = { u: u, edgeIdx: edge.edgeIdx };
                
                let h = 0;
                if (algorithm === 'astar') {
                    h = dist(nodes[v], nodes[targetNode]);
                }
                pq.push({id: v, dist: newDist, priority: newDist + h});
            }
        }
    }

    const computeTime = performance.now() - t0;
    const pathCost = distances[targetNode];

    return { exploredCount, computeTime, pathCost };
}

document.getElementById('btnCompare').addEventListener('click', () => {
    if (sourceNode === -1 || targetNode === -1) {
        alert("Please select or randomize a Source and Target node first!");
        return;
    }
    if (isAnimating) return;

    // Run both algorithms silently (no animation)
    const dijkstraResult = runPathfindingSilent('dijkstra');
    const astarResult = runPathfindingSilent('astar');

    // Populate the comparison table
    document.getElementById('cmpDijkstraExplored').innerText = dijkstraResult.exploredCount;
    document.getElementById('cmpAstarExplored').innerText = astarResult.exploredCount;
    document.getElementById('cmpDijkstraCost').innerText = dijkstraResult.pathCost === Infinity ? 'No Path' : dijkstraResult.pathCost.toFixed(1);
    document.getElementById('cmpAstarCost').innerText = astarResult.pathCost === Infinity ? 'No Path' : astarResult.pathCost.toFixed(1);
    document.getElementById('cmpDijkstraTime').innerText = dijkstraResult.computeTime.toFixed(3) + ' ms';
    document.getElementById('cmpAstarTime').innerText = astarResult.computeTime.toFixed(3) + ' ms';

    // Determine winners
    const winExplored = document.getElementById('cmpWinnerExplored');
    const winCost = document.getElementById('cmpWinnerCost');
    const winTime = document.getElementById('cmpWinnerTime');

    if (dijkstraResult.exploredCount < astarResult.exploredCount) {
        winExplored.innerText = '🏆 Dijkstra';
        winExplored.style.color = '#a855f7';
    } else if (astarResult.exploredCount < dijkstraResult.exploredCount) {
        winExplored.innerText = '🏆 A*';
        winExplored.style.color = '#ffc940';
    } else {
        winExplored.innerText = 'Tie';
        winExplored.style.color = '#6b6e82';
    }

    if (dijkstraResult.pathCost <= astarResult.pathCost + 0.01) {
        winCost.innerText = '🏆 Dijkstra';
        winCost.style.color = '#a855f7';
    } else {
        winCost.innerText = '🏆 A*';
        winCost.style.color = '#ffc940';
    }

    if (dijkstraResult.computeTime < astarResult.computeTime) {
        winTime.innerText = '🏆 Dijkstra';
        winTime.style.color = '#a855f7';
    } else {
        winTime.innerText = '🏆 A*';
        winTime.style.color = '#ffc940';
    }

    // Generate verdict
    const saved = ((1 - astarResult.exploredCount / dijkstraResult.exploredCount) * 100).toFixed(1);
    let verdict = '';
    if (astarResult.exploredCount < dijkstraResult.exploredCount) {
        verdict = `A* explored ${saved}% fewer nodes than Dijkstra thanks to the Euclidean heuristic guiding the search toward the target.`;
    } else {
        verdict = `Dijkstra was more efficient here. The traffic multipliers degraded A*'s heuristic accuracy, causing it to explore more nodes than a blind search.`;
    }
    if (dijkstraResult.computeTime < astarResult.computeTime) {
        verdict += ` However, Dijkstra was ${(astarResult.computeTime / dijkstraResult.computeTime).toFixed(1)}× faster due to lower per-node overhead (no sqrt heuristic).`;
    }
    document.getElementById('cmpVerdict').innerText = verdict;

    // Show overlay
    document.getElementById('compareOverlay').style.display = 'flex';
});

document.getElementById('btnCloseCompare').addEventListener('click', () => {
    document.getElementById('compareOverlay').style.display = 'none';
});

// ================== FEATURE: MULTI-STOP DELIVERY ROUTE ==================

function dijkstraShortestPath(src, tgt) {
    // Returns { cost, path: [edgeIndices] } or null if no path
    const adj = Array.from({length: nodes.length}, () => []);
    edges.forEach((e, idx) => {
        adj[e.u].push({v: e.v, w: e.weight, edgeIdx: idx});
        adj[e.v].push({v: e.u, w: e.weight, edgeIdx: idx});
    });

    const distances = Array(nodes.length).fill(Infinity);
    const parent = Array(nodes.length).fill(null);
    const pq = [];
    distances[src] = 0;
    pq.push({id: src, dist: 0});

    while (pq.length > 0) {
        pq.sort((a, b) => a.dist - b.dist);
        const u = pq.shift().id;
        if (u === tgt) break;
        if (distances[u] > (distances[u] || Infinity) + 0.001) continue;

        for (const edge of adj[u]) {
            const v = edge.v;
            const newDist = distances[u] + edge.w;
            if (newDist < distances[v]) {
                distances[v] = newDist;
                parent[v] = { u, edgeIdx: edge.edgeIdx };
                pq.push({id: v, dist: newDist});
            }
        }
    }

    if (distances[tgt] === Infinity) return null;

    // Backtrack to get path edges
    const pathEdges = [];
    const pathNodes = [tgt];
    let curr = tgt;
    while (curr !== src) {
        const p = parent[curr];
        pathEdges.push(p.edgeIdx);
        pathNodes.push(p.u);
        curr = p.u;
    }
    return { cost: distances[tgt], pathEdges, pathNodes };
}

document.getElementById('btnRandomWaypoints').addEventListener('click', () => {
    if (nodes.length === 0) return;
    const waypoints = new Set();
    while (waypoints.size < Math.min(5, nodes.length)) {
        waypoints.add(Math.floor(Math.random() * nodes.length));
    }
    document.getElementById('waypointInput').value = Array.from(waypoints).join(', ');
});

document.getElementById('btnDeliveryRoute').addEventListener('click', async () => {
    const input = document.getElementById('waypointInput').value;
    const waypoints = input.split(',').map(s => parseInt(s.trim())).filter(n => !isNaN(n) && n >= 0 && n < nodes.length);
    
    if (waypoints.length < 2) {
        alert("Please enter at least 2 valid waypoint IDs (comma separated).");
        alert("Please enter at least 2 waypoint IDs.");
        return;
    }
    if (waypoints.length > 20) {
        alert("Maximum allowed waypoints is 20 for exact DP calculation.");
        return;
    }
    if (isAnimating) return;

    resetGraphState();
    isAnimating = true;
    stopAnimation = false;

    // Mark all waypoints
    waypoints.forEach(w => { nodes[w].state = 'waypoint'; });
    draw();
    statExplored.innerText = `Computing DP...`;
    
    // Allow UI to update before heavy computation
    await new Promise(r => setTimeout(r, 50));

    const t0 = performance.now();
    const N = waypoints.length;

    // 1. Build Distance Matrix (All-Pairs Shortest Paths for waypoints)
    const distMatrix = Array(N).fill(0).map(() => Array(N).fill(Infinity));
    const pathEdgesMatrix = Array(N).fill(0).map(() => Array(N).fill(null));
    const pathNodesMatrix = Array(N).fill(0).map(() => Array(N).fill(null));

    for (let i = 0; i < N; i++) {
        for (let j = i + 1; j < N; j++) {
            const res = dijkstraShortestPath(waypoints[i], waypoints[j]);
            if (res) {
                distMatrix[i][j] = res.cost;
                distMatrix[j][i] = res.cost;
                pathEdgesMatrix[i][j] = res.pathEdges;
                pathEdgesMatrix[j][i] = [...res.pathEdges].reverse(); // reverse edges for undirected
                pathNodesMatrix[i][j] = res.pathNodes;
                pathNodesMatrix[j][i] = [...res.pathNodes].reverse();
            } else {
                alert(`No path found between waypoint ${waypoints[i]} and ${waypoints[j]}! Graph is disconnected.`);
                isAnimating = false;
                return;
            }
        }
    }

    // 2. Held-Karp Algorithm (Dynamic Programming with Bitmasking)
    // dp[mask][i] = min cost to visit subset 'mask', ending at waypoint 'i'
    const totalStates = 1 << N;
    const dp = new Float32Array(totalStates * N).fill(Infinity);
    const parent = new Int32Array(totalStates * N).fill(-1);

    // Start at waypoint 0
    dp[1 * N + 0] = 0; 

    for (let mask = 1; mask < totalStates; mask++) {
        // We must always have visited the start node (0th bit must be 1)
        if ((mask & 1) === 0) continue;

        for (let u = 0; u < N; u++) {
            // If u is not in the current mask, skip
            if ((mask & (1 << u)) === 0) continue;
            
            const currentDist = dp[mask * N + u];
            if (currentDist === Infinity) continue;

            for (let v = 0; v < N; v++) {
                // If v is already in the mask, skip
                if ((mask & (1 << v)) !== 0) continue;

                const nextMask = mask | (1 << v);
                const newDist = currentDist + distMatrix[u][v];
                
                const nextIdx = nextMask * N + v;
                if (newDist < dp[nextIdx]) {
                    dp[nextIdx] = newDist;
                    parent[nextIdx] = u;
                }
            }
        }
    }

    // Find the best ending node
    const finalMask = totalStates - 1;
    let minPathCost = Infinity;
    let bestEndNode = -1;

    for (let i = 1; i < N; i++) {
        const cost = dp[finalMask * N + i];
        if (cost < minPathCost) {
            minPathCost = cost;
            bestEndNode = i;
        }
    }

    // 3. Backtrack to find the optimal order
    const visitOrder = [];
    let currMask = finalMask;
    let currNode = bestEndNode;

    while (currNode !== -1) {
        visitOrder.push(currNode);
        const prevNode = parent[currMask * N + currNode];
        currMask = currMask ^ (1 << currNode);
        currNode = prevNode;
    }
    visitOrder.reverse(); // Now it's 0 -> ... -> bestEndNode

    const computeTime = performance.now() - t0;
    
    console.log(`Held-Karp executed for N=${N} in ${computeTime.toFixed(2)}ms`);

    // 4. Animate the final route
    let totalCost = 0;
    let totalEdges = 0;
    for (let i = 0; i < visitOrder.length - 1; i++) {
        if (stopAnimation) break;
        
        const srcIdx = visitOrder[i];
        const tgtIdx = visitOrder[i + 1];
        
        totalCost += distMatrix[srcIdx][tgtIdx];

        const edgesToAnim = pathEdgesMatrix[srcIdx][tgtIdx];
        const nodesToAnim = pathNodesMatrix[srcIdx][tgtIdx];

        // Animate this segment
        for (const edgeIdx of edgesToAnim) {
            edges[edgeIdx].state = 'path';
            totalEdges++;
        }
        for (const nodeId of nodesToAnim) {
            if (waypoints.includes(nodeId)) {
                nodes[nodeId].state = 'waypoint';
            } else {
                nodes[nodeId].state = 'path';
            }
        }

        statExplored.innerText = `Leg ${i + 1}/${visitOrder.length - 1}`;
        statLength.innerText = totalCost.toFixed(0);
        draw();
        await sleep(400);
    }

    statTime.innerText = computeTime.toFixed(2);
    statExplored.innerText = `${visitOrder.length} stops, ${totalEdges} roads`;
    statLength.innerText = totalCost.toFixed(0);

    isAnimating = false;
    draw();
});
