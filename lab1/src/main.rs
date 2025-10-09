use std::fmt;

#[derive(Clone)]
struct Bill {
    limiting_amount: f64,
    current_debt: f64,
}

impl Bill {
    fn new(limiting_amount: f64) -> Self {
        Self {
            limiting_amount,
            current_debt: 0.0,
        }
    }

    fn check(&self, amount: f64) -> bool {
        (self.current_debt + amount) <= self.limiting_amount + 1e-9
    }

    fn add(&mut self, amount: f64) {
        self.current_debt += amount;
    }

    fn pay(&mut self, amount: f64) {
        self.current_debt -= amount;
        if self.current_debt < 0.0 {
            self.current_debt = 0.0;
        }
    }

    fn change_the_limit(&mut self, amount: f64) {
        self.limiting_amount = amount;
    }

    fn get_limiting_amount(&self) -> f64 {
        self.limiting_amount
    }
    fn get_current_debt(&self) -> f64 {
        self.current_debt
    }
}

impl fmt::Display for Bill {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(
            f,
            "Bill(limit: {:.2}, debt: {:.2})",
            self.limiting_amount, self.current_debt
        )
    }
}

#[derive(Clone)]
struct Operator {
    id: usize,
    talking_charge: f64,  // per minute
    message_cost: f64,    // per message
    network_charge: f64,  // per MB
    discount_rate: i32,   // percent (e.g., 10 means 10%)
}

impl Operator {
    fn new(id: usize, talking_charge: f64, message_cost: f64, network_charge: f64, discount_rate: i32) -> Self {
        Self {
            id,
            talking_charge,
            message_cost,
            network_charge,
            discount_rate,
        }
    }

    fn calculate_talking_cost(&self, minute: i32, customer_age: usize) -> f64 {
        let base = self.talking_charge * (minute as f64);
        let mut cost = base;
        if customer_age < 18 || customer_age > 65 {
            let d = (self.discount_rate as f64) / 100.0;
            cost = base * (1.0 - d);
        }
        cost
    }

    fn calculate_message_cost(&self, quantity: i32, same_operator: bool) -> f64 {
        let base = self.message_cost * (quantity as f64);
        let mut cost = base;
        if same_operator {
            let d = (self.discount_rate as f64) / 100.0;
            cost = base * (1.0 - d);
        }
        cost
    }

    fn calculate_network_cost(&self, amount: f64) -> f64 {
        self.network_charge * amount
    }

    fn get_talking_charge(&self) -> f64 { self.talking_charge }
    fn set_talking_charge(&mut self, v: f64) { self.talking_charge = v; }
    fn get_message_cost(&self) -> f64 { self.message_cost }
    fn set_message_cost(&mut self, v: f64) { self.message_cost = v; }
    fn get_network_charge(&self) -> f64 { self.network_charge }
    fn set_network_charge(&mut self, v: f64) { self.network_charge = v; }
    fn get_discount_rate(&self) -> i32 { self.discount_rate }
    fn set_discount_rate(&mut self, v: i32) { self.discount_rate = v; }
}

impl fmt::Display for Operator {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(
            f,
            "Operator(id: {}, talk: {:.3}/min, msg: {:.3}/msg, net: {:.3}/MB, disc: {}%)",
            self.id, self.talking_charge, self.message_cost, self.network_charge, self.discount_rate
        )
    }
}

struct Customer {
    id: usize,
    name: String,
    age: usize,
    operator_index: usize, // index into operators array
    bill_index: usize,     // index into bills array
}

impl Customer {
    fn new(id: usize, name: &str, age: usize, operator_index: usize, bill_index: usize) -> Self {
        Self {
            id,
            name: name.to_string(),
            age,
            operator_index,
            bill_index,
        }
    }

    fn talk(&self, minute: i32, other: &Customer, operators: &Vec<Option<Operator>>, bills: &mut Vec<Option<Bill>>) {
        // retrieve operator for self
        let op = operators[self.operator_index].as_ref().expect("Operator missing");
        let cost = op.calculate_talking_cost(minute, self.age);

        // check bill
        let bill = bills[self.bill_index].as_mut().expect("Bill missing");
        if bill.check(cost) {
            bill.add(cost);
            println!("{} talked to {} for {} min. Cost {:.2} added to bill {}.", self.name, other.name, minute, cost, self.bill_index);
        } else {
            println!("{} wanted to talk for {} min (cost {:.2}) but limit exceeded. No action taken.", self.name, minute, cost);
        }
    }

    // void message(int quantity, Customer other)
    fn message(&self, quantity: i32, other: &Customer, operators: &Vec<Option<Operator>>, bills: &mut Vec<Option<Bill>>) {
        let op_self = operators[self.operator_index].as_ref().expect("Operator missing");
        let same_operator = self.operator_index == other.operator_index;
        let cost = op_self.calculate_message_cost(quantity, same_operator);

        let bill = bills[self.bill_index].as_mut().expect("Bill missing");
        if bill.check(cost) {
            bill.add(cost);
            println!("{} sent {} messages to {}. Cost {:.2} added to bill {}.", self.name, quantity, other.name, cost, self.bill_index);
        } else {
            println!("{} wanted to send {} messages (cost {:.2}) but limit exceeded. No action taken.", self.name, quantity, cost);
        }
    }

    // void connection(double amount) // amount = MB
    fn connection(&self, amount: f64, operators: &Vec<Option<Operator>>, bills: &mut Vec<Option<Bill>>) {
        let op = operators[self.operator_index].as_ref().expect("Operator missing");
        let cost = op.calculate_network_cost(amount);

        let bill = bills[self.bill_index].as_mut().expect("Bill missing");
        if bill.check(cost) {
            bill.add(cost);
            println!("{} used {:.2} MB. Cost {:.2} added to bill {}.", self.name, amount, cost, self.bill_index);
        } else {
            println!("{} wanted to use {:.2} MB (cost {:.2}) but limit exceeded. No action taken.", self.name, amount, cost);
        }
    }

    fn get_age(&self) -> usize { self.age }
    fn set_age(&mut self, v: usize) { self.age = v; }

    fn get_operator_index(&self) -> usize { self.operator_index }
    fn set_operator_index(&mut self, v: usize) { self.operator_index = v; }

    fn get_bill_index(&self) -> usize { self.bill_index }
    fn set_bill_index(&mut self, v: usize) { self.bill_index = v; }
}

impl fmt::Display for Customer {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(
            f,
            "Customer(id: {}, name: {}, age: {}, op: {}, bill: {})",
            self.id, self.name, self.age, self.operator_index, self.bill_index
        )
    }
}



// ---| Helper functions |--- //

fn create_operator_list() -> Vec<Option<Operator>> {
    vec![
        Some(Operator::new(0, 0.5, 0.1, 0.01, 10)), // Operator 0
        Some(Operator::new(1, 0.7, 0.08, 0.015, 5)), // Operator 1
    ]
}

fn create_bill_list() -> Vec<Option<Bill>> {
    vec![
        Some(Bill::new(50.0)), // Bill 0
        Some(Bill::new(100.0)), // Bill 1
        Some(Bill::new(30.0)), // Bill 2
    ]
}

fn create_customers() -> Vec<Option<Customer>> {
    vec![
        Some(Customer::new(0, "Alice", 17, 0, 0)), // under 18 => age discount applies on talk
        Some(Customer::new(1, "Bob", 30, 1, 1)),
        Some(Customer::new(2, "Carol", 70, 0, 2)), // over 65 => age discount applies
    ]
}

fn print_state(customers: &Vec<Option<Customer>>, operators: &Vec<Option<Operator>>, bills: &Vec<Option<Bill>>) {
    println!("=== Operators ===");
    for (i, op) in operators.iter().enumerate() {
        if let Some(op) = op {
            println!("op[{}] = {}", i, op);
        } else {
            println!("op[{}] = None", i);
        }
    }

    println!("\n=== Bills ===");
    for (i, b) in bills.iter().enumerate() {
        if let Some(b) = b {
            println!("bill[{}] = {}", i, b);
        } else {
            println!("bill[{}] = None", i);
        }
    }

    println!("\n=== Customers ===");
    for (i, c) in customers.iter().enumerate() {
        if let Some(c) = c {
            println!("cust[{}] = {}", i, c);
        } else {
            println!("cust[{}] = None", i);
        }
    }
    println!("=================\n");
}

fn main() {
    let mut operators: Vec<Option<Operator>> = Vec::new();
    let mut bills: Vec<Option<Bill>> = Vec::new();
    let mut customers: Vec<Option<Customer>> = Vec::new();

    operators = create_operator_list();
    bills = create_bill_list();
    customers = create_customers();

    println!("Initial State:");
    print_state(&customers, &operators, &bills);

    {
        let alice = customers[0].as_ref().unwrap().clone();
        let bob = customers[1].as_ref().unwrap().clone();
        alice.talk(10, &bob, &operators, &mut bills);
    }

    {
        let bob = customers[1].as_ref().unwrap().clone();
        let alice = customers[0].as_ref().unwrap().clone();
        bob.message(5, &alice, &operators, &mut bills);
    }

    // 5. A customer can connect to the internet;
    // Carol (2) uses 200 MB
    {
        let carol = customers[2].as_ref().unwrap().clone();
        carol.connection(200.0, &operators, &mut bills);
    }

    // 6. A customer can pay his/her bills;
    // Pay from Bill 0: pay 3.0
    {
        let b = bills[0].as_mut().unwrap();
        println!("Paying 3.0 towards bill[0]. Old debt: {:.2}", b.get_current_debt());
        b.pay(3.0);
        println!("New debt: {:.2}", b.get_current_debt());
    }

    // 7. A customer can change his/her operator;
    // Bob (1) switches to operator 0
    {
        let cust_mut = customers[1].as_mut().unwrap();
        println!("Bob switching from operator {} to operator 0", cust_mut.get_operator_index());
        cust_mut.set_operator_index(0);
    }

    // 8. A customer can change his/her Bill limit.
    // Increase Bob's bill limit (bill index 1) to 200.0
    {
        let b = bills[1].as_mut().unwrap();
        println!("Changing bill[1] limit from {:.2} to 200.00", b.get_limiting_amount());
        b.change_the_limit(200.0);
    }

    println!("\nState after operations:");
    print_state(&customers, &operators, &bills);

    // Additional demonstration: attempt an action that exceeds the limit
    // Try to have Alice (bill 0) consume a large connection that would exceed her limit
    {
        let alice = customers[0].as_ref().unwrap().clone();
        println!("Attempting a large connection for Alice that should exceed limit:");
        alice.connection(10000.0, &operators, &mut bills);
    }

    println!("\nFinal state:");
    print_state(&customers, &operators, &bills);

    println!("Demo complete.");
}
