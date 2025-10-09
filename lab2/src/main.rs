use serde::{Deserialize, Serialize};
use std::collections::{HashMap, HashSet};
use std::fs;
use std::f64::consts::PI;

fn haversine_km(lat1: f64, lon1: f64, lat2: f64, lon2: f64) -> f64 {
    // inputs in degrees -> returns kilometers
    let to_rad = |d: f64| d * PI / 180.0;
    let r = 6371.0_f64; // earth radius in km
    let (lat1r, lon1r, lat2r, lon2r) = (to_rad(lat1), to_rad(lon1), to_rad(lat2), to_rad(lon2));
    let dlat = lat2r - lat1r;
    let dlon = lon2r - lon1r;
    let a = (dlat / 2.0).sin().powi(2) + lat1r.cos() * lat2r.cos() * (dlon / 2.0).sin().powi(2);
    let c = 2.0 * a.sqrt().asin();
    r * c
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(tag = "type")]
pub enum ContainerData {
    Basic { id: usize, weight: i32 },
    Heavy { id: usize, weight: i32 },
    Refrigerated { id: usize, weight: i32 },
    Liquid { id: usize, weight: i32 },
}

impl ContainerData {
    pub fn id(&self) -> usize {
        match self {
            ContainerData::Basic { id, .. } => *id,
            ContainerData::Heavy { id, .. } => *id,
            ContainerData::Refrigerated { id, .. } => *id,
            ContainerData::Liquid { id, .. } => *id,
        }
    }
    pub fn weight(&self) -> i32 {
        match self {
            ContainerData::Basic { weight, .. } => *weight,
            ContainerData::Heavy { weight, .. } => *weight,
            ContainerData::Refrigerated { weight, .. } => *weight,
            ContainerData::Liquid { weight, .. } => *weight,
        }
    }
    pub fn consumption_per_unit(&self) -> f64 {
        match self {
            ContainerData::Basic { .. } => 2.50,
            ContainerData::Heavy { .. } => 3.00,
            ContainerData::Refrigerated { .. } => 5.00,
            ContainerData::Liquid { .. } => 4.00,
        }
    }
    pub fn total_consumption(&self) -> f64 {
        self.consumption_per_unit() * (self.weight() as f64)
    }
    pub fn kind_name(&self) -> &'static str {
        match self {
            ContainerData::Basic { .. } => "basic",
            ContainerData::Heavy { .. } => "heavy",
            ContainerData::Refrigerated { .. } => "refrigerated",
            ContainerData::Liquid { .. } => "liquid",
        }
    }
}

pub trait IPort {
    fn incoming_ship(&mut self, s_id: usize); // add to current if not present
    fn outgoing_ship(&mut self, s_id: usize); // add to history if not duplicate
}

pub trait IShip {
    fn sail_to(&mut self, dest_port_id: usize, ports: &mut HashMap<usize, Port>, ships: &mut HashMap<usize, Ship>) -> bool;
    fn re_fuel(&mut self, amount: f64);
    fn load(&mut self, cont_id: usize, ports: &mut HashMap<usize, Port>, container_store: &mut HashMap<usize, ContainerData>) -> bool;
    fn un_load(&mut self, cont_id: usize, ports: &mut HashMap<usize, Port>, container_store: &mut mut_ref) -> bool;
}

type mut_ref = HashMap<usize, ContainerData>;

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Port {
    pub id: usize,
    pub latitude: f64,
    pub longitude: f64,
    #[serde(skip)]
    pub containers: HashSet<usize>, // container IDs present in port
    #[serde(skip)]
    pub history: HashSet<usize>,    // ship IDs that visited
    #[serde(skip)]
    pub current: HashSet<usize>,    // ship IDs currently here
}

impl Port {
    pub fn new(id: usize, latitude: f64, longitude: f64) -> Self {
        Self { id, latitude, longitude, containers: HashSet::new(), history: HashSet::new(), current: HashSet::new() }
    }
    pub fn get_distance(&self, other: &Port) -> f64 {
        haversine_km(self.latitude, self.longitude, other.latitude, other.longitude)
    }
}

impl IPort for Port {
    fn incoming_ship(&mut self, s_id: usize) {
        self.current.insert(s_id);
        self.history.insert(s_id);
    }
    fn outgoing_ship(&mut self, s_id: usize) {
        self.current.remove(&s_id);
        // history remains (do not duplicate)
        self.history.insert(s_id);
    }
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Ship {
    pub id: usize,
    pub fuel: f64,
    pub current_port: usize,
    pub total_weight_capacity: i32,
    pub max_number_of_all_containers: usize,
    pub max_number_of_heavy_containers: usize,
    pub max_number_of_refrigerated_containers: usize,
    pub max_number_of_liquid_containers: usize,
    pub fuel_consumption_per_km: f64,

    #[serde(skip)]
    pub containers: Vec<usize>, // container IDs currently on ship
}

impl Ship {
    pub fn new(
        id: usize,
        current_port: usize,
        total_weight_capacity: i32,
        max_number_of_all_containers: usize,
        max_number_of_heavy_containers: usize,
        max_number_of_refrigerated_containers: usize,
        max_number_of_liquid_containers: usize,
        fuel_consumption_per_km: f64,
    ) -> Self {
        Self {
            id,
            fuel: 0.0,
            current_port,
            total_weight_capacity,
            max_number_of_all_containers,
            max_number_of_heavy_containers,
            max_number_of_refrigerated_containers,
            max_number_of_liquid_containers,
            fuel_consumption_per_km,
            containers: Vec::new(),
        }
    }

    pub fn get_current_containers_sorted(&self) -> Vec<usize> {
        let mut v = self.containers.clone();
        v.sort_unstable();
        v
    }

    fn current_total_weight(&self, container_store: &HashMap<usize, ContainerData>) -> i32 {
        self.containers.iter().map(|id| container_store.get(id).unwrap().weight()).sum()
    }
    fn current_heavy_count(&self, container_store: &HashMap<usize, ContainerData>) -> usize {
        self.containers.iter().filter(|id| matches!(container_store.get(id).unwrap(), ContainerData::Heavy {..} | ContainerData::Refrigerated {..} | ContainerData::Liquid {..})).count()
    }
    fn current_refrigerated_count(&self, container_store: &HashMap<usize, ContainerData>) -> usize {
        self.containers.iter().filter(|id| matches!(container_store.get(id).unwrap(), ContainerData::Refrigerated {..})).count()
    }
    fn current_liquid_count(&self, container_store: &HashMap<usize, ContainerData>) -> usize {
        self.containers.iter().filter(|id| matches!(container_store.get(id).unwrap(), ContainerData::Liquid {..})).count()
    }
}

impl IShip for Ship {
    fn sail_to(&mut self, dest_port_id: usize, ports: &mut HashMap<usize, Port>, ships: &mut HashMap<usize, Ship>) -> bool {
        // calculate distance
        let current_port = ports.get(&self.current_port).unwrap().clone();
        let dest_port = match ports.get(&dest_port_id) {
            Some(p) => p.clone(),
            None => return false,
        };
        let km = current_port.get_distance(&dest_port);
        // base consumption
        let mut required = km * self.fuel_consumption_per_km;
        if self.fuel >= required {
            ports.get_mut(&self.current_port).unwrap().outgoing_ship(self.id);
            self.fuel -= required;
            self.current_port = dest_port_id;
            ports.get_mut(&dest_port_id).unwrap().incoming_ship(self.id);
            true
        } else {
            false
        }
    }

    fn re_fuel(&mut self, amount: f64) {
        self.fuel += amount;
    }

    fn load(&mut self, cont_id: usize, ports: &mut HashMap<usize, Port>, container_store: &mut HashMap<usize, ContainerData>) -> bool {
        // check container exists in port
        let port = ports.get_mut(&self.current_port).unwrap();
        if !port.containers.contains(&cont_id) {
            return false;
        }
        // capacity checks
        let cont = container_store.get(&cont_id).unwrap();
        let new_weight = self.current_total_weight(container_store) + cont.weight();
        if new_weight > self.total_weight_capacity {
            return false;
        }
        if self.containers.len() + 1 > self.max_number_of_all_containers {
            return false;
        }
        let heavy_count = self.current_heavy_count(container_store) + if matches!(cont, ContainerData::Heavy {..} | ContainerData::Refrigerated {..} | ContainerData::Liquid {..}) {1} else {0};
        if heavy_count > self.max_number_of_heavy_containers {
            return false;
        }
        let ref_count = self.current_refrigerated_count(container_store) + if matches!(cont, ContainerData::Refrigerated{..}) {1} else {0};
        if ref_count > self.max_number_of_refrigerated_containers {
            return false;
        }
        let liq_count = self.current_liquid_count(container_store) + if matches!(cont, ContainerData::Liquid{..}) {1} else {0};
        if liq_count > self.max_number_of_liquid_containers {
            return false;
        }
        port.containers.remove(&cont_id);
        self.containers.push(cont_id);
        true
    }

    fn un_load(&mut self, cont_id: usize, ports: &mut HashMap<usize, Port>, container_store: &mut mut_ref) -> bool {
        if let Some(pos) = self.containers.iter().position(|&x| x == cont_id) {
            self.containers.swap_remove(pos);
            ports.get_mut(&self.current_port).unwrap().containers.insert(cont_id);
            return true;
        }
        false
    }
}

#[derive(Deserialize)]
#[serde(rename_all = "lowercase")]
enum Operation {
    CreatePort { id: usize, latitude: f64, longitude: f64 },
    CreateShip { id: usize, port_id: usize, total_weight_capacity: i32, max_number_of_all_containers: usize, max_number_of_heavy_containers: usize, max_number_of_refrigerated_containers: usize, max_number_of_liquid_containers: usize, fuel_consumption_per_km: f64 },
    CreateContainer { id: usize, weight: i32, special: Option<String>, port_id: usize },
    Load { ship_id: usize, container_id: usize },
    Unload { ship_id: usize, container_id: usize },
    Sail { ship_id: usize, dest_port_id: usize },
    Refuel { ship_id: usize, amount: f64 },
}

#[derive(Deserialize)]
struct InputFile {
    operations: Vec<Operation>,
}

#[derive(Serialize)]
struct OutputPort {
    lat: f64,
    lon: f64,
    basic_container: Vec<usize>,
    heavy_container: Vec<usize>,
    refrigerated_container: Vec<usize>,
    liquid_container: Vec<usize>,
    ships: HashMap<String, OutputShip>,
}

#[derive(Serialize)]
struct OutputShip {
    fuel_left: f64,
    basic_container: Vec<usize>,
    heavy_container: Vec<usize>,
    refrigerated_container: Vec<usize>,
    liquid_container: Vec<usize>,
}

fn main() {
    // for demo: read "input.json" from current directory
    let input_text = fs::read_to_string("input.json").expect("input.json not found");
    let input: InputFile = serde_json::from_str(&input_text).expect("invalid JSON");

    let mut ports: HashMap<usize, Port> = HashMap::new();
    let mut ships: HashMap<usize, Ship> = HashMap::new();
    let mut container_store: HashMap<usize, ContainerData> = HashMap::new();

    for op in input.operations {
        match op {
            Operation::CreatePort { id, latitude, longitude } => {
                ports.insert(id, Port::new(id, latitude, longitude));
            }
            Operation::CreateShip { id, port_id, total_weight_capacity, max_number_of_all_containers, max_number_of_heavy_containers, max_number_of_refrigerated_containers, max_number_of_liquid_containers, fuel_consumption_per_km } => {
                let mut s = Ship::new(id, port_id, total_weight_capacity, max_number_of_all_containers, max_number_of_heavy_containers, max_number_of_refrigerated_containers, max_number_of_liquid_containers, fuel_consumption_per_km);
                // place ship at port (port must exist)
                if let Some(p) = ports.get_mut(&port_id) {
                    p.incoming_ship(id);
                }
                ships.insert(id, s);
            }
            Operation::CreateContainer { id, weight, special, port_id } => {
                let cont = match special.as_deref() {
                    Some("R") => ContainerData::Refrigerated { id, weight },
                    Some("L") => ContainerData::Liquid { id, weight },
                    _ => {
                        if weight <= 3000 { ContainerData::Basic { id, weight } } else { ContainerData::Heavy { id, weight } }
                    }
                };
                container_store.insert(id, cont);
                if let Some(p) = ports.get_mut(&port_id) {
                    p.containers.insert(id);
                } else {
                    // ignore or create port? choose to create a port placeholder at 0,0
                    let mut p = Port::new(port_id, 0.0, 0.0);
                    p.containers.insert(id);
                    ports.insert(port_id, p);
                }
            }
            Operation::Load { ship_id, container_id } => {
                if let Some(ship) = ships.get_mut(&ship_id) {
                    let ok = ship.load(container_id, &mut ports, &mut container_store);
                    if !ok {
                        // load failed: ignore or log; here we ignore
                    }
                }
            }
            Operation::Unload { ship_id, container_id } => {
                if let Some(ship) = ships.get_mut(&ship_id) {
                    let _ = ship.un_load(container_id, &mut ports, &mut container_store);
                }
            }
            Operation::Refuel { ship_id, amount } => {
                if let Some(ship) = ships.get_mut(&ship_id) {
                    ship.re_fuel(amount);
                }
            }
            Operation::Sail { ship_id, dest_port_id } => {
                // compute total consumption including containers
                if let Some(ship) = ships.get(&ship_id) {
                    let current_port = ports.get(&ship.current_port).unwrap().clone();
                    let dest = match ports.get(&dest_port_id) {
                        Some(p) => p.clone(),
                        None => continue,
                    };
                    let km = current_port.get_distance(&dest);
                    // container consumption:
                    let container_consumption: f64 = ship.containers.iter().map(|cid| container_store.get(cid).unwrap().total_consumption()).sum();
                    let required = km * ship.fuel_consumption_per_km + container_consumption;
                    // attempt to sail
                    let ship_mut = ships.get_mut(&ship_id).unwrap();
                    if ship_mut.fuel >= required {
                        // sufficient fuel
                        let _ = ship_mut.sail_to(dest_port_id, &mut ports, &mut ships);
                        // NOTE: sail_to consumes only base fuel; we subtract container consumption here to reflect actual consumption
                        ship_mut.fuel -= container_consumption;
                    } else {
                        // find nearest port to current to refuel
                        let mut nearest_id: Option<usize> = None;
                        let mut nearest_dist = std::f64::MAX;
                        for (&pid, p) in ports.iter() {
                            if pid == ship.current_port { continue; }
                            let d = current_port.get_distance(p);
                            if d < nearest_dist {
                                nearest_dist = d;
                                nearest_id = Some(pid);
                            }
                        }
                        if let Some(npid) = nearest_id {
                            // sail to nearest if we have enough fuel for that leg (compute cost first leg + container consumption)
                            let leg_km = current_port.get_distance(&ports.get(&npid).unwrap());
                            let req_leg = leg_km * ship.fuel_consumption_per_km + container_consumption;
                            if ship_mut.fuel >= req_leg {
                                let _ = ship_mut.sail_to(npid, &mut ports, &mut ships);
                                ship_mut.fuel -= container_consumption; // subtract container consumption for that leg
                                // then refuel full arbitrary amount (for simplicity add a big amount)
                                ship_mut.re_fuel(10000.0);
                                // finally attempt to sail to dest
                                let current_port_after = ports.get(&ship_mut.current_port).unwrap().clone();
                                let km2 = current_port_after.get_distance(&dest);
                                let required2 = km2 * ship_mut.fuel_consumption_per_km + container_consumption;
                                if ship_mut.fuel >= required2 {
                                    let _ = ship_mut.sail_to(dest_port_id, &mut ports, &mut ships);
                                    ship_mut.fuel -= container_consumption;
                                }
                            } else {
                                // cannot reach nearest port - do nothing
                            }
                        }
                    }
                }
            }
        }
    }

    let mut out_map: serde_json::Map<String, serde_json::Value> = serde_json::Map::new();
    let mut port_ids: Vec<_> = ports.keys().cloned().collect();
    port_ids.sort_unstable();
    for pid in port_ids {
        let p = ports.get(&pid).unwrap();
        // containers by kind
        let mut basic = Vec::new();
        let mut heavy = Vec::new();
        let mut refrigerated = Vec::new();
        let mut liquid = Vec::new();
        for &cid in p.containers.iter() {
            let cont = container_store.get(&cid).unwrap();
            match cont {
                ContainerData::Basic { .. } => basic.push(cid),
                ContainerData::Heavy { .. } => heavy.push(cid),
                ContainerData::Refrigerated { .. } => refrigerated.push(cid),
                ContainerData::Liquid { .. } => liquid.push(cid),
            }
        }
        basic.sort_unstable();
        heavy.sort_unstable();
        refrigerated.sort_unstable();
        liquid.sort_unstable();
        let mut ship_map = serde_json::Map::new();
        let mut ship_ids: Vec<_> = p.current.iter().cloned().collect();
        ship_ids.sort_unstable();
        for sid in ship_ids {
            let s = ships.get(&sid).unwrap();
            let mut sbasic = Vec::new();
            let mut sheavy = Vec::new();
            let mut sref = Vec::new();
            let mut sliq = Vec::new();
            for &cid in s.get_current_containers_sorted().iter() {
                let cont = container_store.get(&cid).unwrap();
                match cont {
                    ContainerData::Basic { .. } => sbasic.push(cid),
                    ContainerData::Heavy { .. } => sheavy.push(cid),
                    ContainerData::Refrigerated { .. } => sref.push(cid),
                    ContainerData::Liquid { .. } => sliq.push(cid),
                }
            }
            let sjson = serde_json::json!({
                "fuel_left": (s.fuel * 100.0).round() / 100.0, // 2 decimals
                "basic_container": sbasic,
                "heavy_container": sheavy,
                "refrigerated_container": sref,
                "liquid_container": sliq
            });
            ship_map.insert(format!("ship_{}", sid), sjson);
        }

        let port_json = serde_json::json!({
            "lat": (p.latitude * 100.0).round() / 100.0,
            "lon": (p.longitude * 100.0).round() / 100.0,
            "basic_container": basic,
            "heavy_container": heavy,
            "refrigerated_container": refrigerated,
            "liquid_container": liquid,
            "ships": ship_map
        });
        out_map.insert(format!("Port {}", pid), port_json);
    }

    let out_value = serde_json::Value::Object(out_map);
    let out_text = serde_json::to_string_pretty(&out_value).unwrap();
    fs::write("output.json", out_text).expect("unable to write output.json");
    println!("Finished. Wrote output.json");
}
