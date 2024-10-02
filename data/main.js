const express = require('express');
const mysql = require('mysql2');
const app = express();
const path = require('path');
const fetch = require('node-fetch'); // Use to communicate with NodeMCU

// MySQL database connection
const connection = mysql.createConnection({
    host: 'localhost',
    user: 'root',
    password: 'workbench',
    database: 'cuckoo',
});

connection.connect((err) => {
    if (err) throw err;
    console.log('Connected to MySQL database');
});

// Serve static files
app.use(express.static(path.join(__dirname, 'views')));

app.use(express.json()); // Add this middleware to parse JSON bodies


app.set('view engine', 'ejs');
app.use(express.urlencoded({ extended: false }));
let message = false;

// Render EJS pages
app.get('/', (req, res) => {
    res.render('index', { message });
});

app.post('/signup', (req, res) => {
    const { name, email, password, confirmpassword } = req.body;

    if (!name || !password || !confirmpassword || !email) {
        return res.status(400).send("Invalid Input");
    }
    if (password === confirmpassword) {
        connection.query('INSERT INTO cuckoo(User_Name, Email_Id, Password, Confirm_Password) values(?, ?, ?, ?)', 
            [name, email, password, confirmpassword], (err, result) => {
                if (err) {
                    console.error(err);
                    return res.status(500).send('Error inserting user');
                }
                res.render('timer', { name });
            });
    }
});

app.post('/signin', (req, res) => {
    const { name, password } = req.body;
    if (!name || !password) {
        return res.status(400).send("Invalid Input");
    }

    connection.query('SELECT * FROM cuckoo WHERE User_Name = (?) AND Password = (?)', [name, password], (err, results) => {
        if (err) throw err;

        if (results.length > 0) {
            const name = results[0].User_Name;
            console.log(results);
            res.render('timer', { name });
        } else {
            message = "Wrong credentials, try again";
            res.render('index', { message });
        }
    });
    //res.render('timer', {name:"Vansh"});
});

// Communicate with NodeMCU from Node.js
app.post('/sendToNodeMCU', async (req, res) => {
    

    const { action } = req.body; // Extract the action from the request body
    console.log('Action received:', action);

    try {
        // Make a fetch call to the NodeMCU based on the action
        const response = await fetch(`http://192.168.4.1:80/${action}`, {
            method: 'GET', // Assuming you use GET for this action
            headers: {
                'Content-Type': 'application/json'
            }
        });

        // Check if the response is okay
        if (!response.ok) {
            console.error('HTTP error:', response.status, response.statusText);
            return res.status(response.status).send('Error calling NodeMCU');
        }

        // Get the response text from NodeMCU
        const data = await response.text(); // NodeMCU's response, e.g., "Motor is moving forward"
        console.log('Response from NodeMCU:', data);

        // Send the data back to the client
        res.send(data); // Send the response from the NodeMCU to the client
    } catch (error) {
        console.error('Error fetching from NodeMCU:', error);
        res.status(500).send('Internal server error');
    }
});


app.get('/sendTimeToNodeMCU', async (req, res) => {
    const { set_time } = req.query; // Extract the set_time from query parameters
    console.log('Time interval received:', set_time);

    try {
        // Construct the URL where the value of set_time is passed as part of the URL
        // For example: http://192.168.4.1:80/set_time?value=x where x is the time interval
        const response = await fetch(`http://192.168.4.1:80/set_time`, {
            method: 'POST',
            headers: {
              'Content-Type': 'application/json',
            },
            body: JSON.stringify({ set_time: set_time }),
          });

        // Check if the response is okay
        if (!response.ok) {
            console.error('HTTP error:', response.status, response.statusText);
            return res.status(response.status).send('Error calling NodeMCU');
        }

        // Get the response text from NodeMCU
        const data = await response.text(); // NodeMCU's response, e.g., "Motor is moving forward"
        console.log('Response from NodeMCU:', data);

        // Send the NodeMCU's response back to the client
        res.send(data); // Send the response from the NodeMCU to the client

    } catch (error) {
        console.error('Error fetching from NodeMCU:', error);
        res.status(500).send('Internal server error');
    }
});


const PORT = process.env.PORT || 3003;
app.listen(PORT, () => console.log(`Server running on port ${PORT}`));
