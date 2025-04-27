const express = require('express');
const mysql = require('mysql2');
const app = express();
const axios = require('axios');


const path = require('path');
const fetch = require('node-fetch'); // Use to communicate with NodeMCU

const NODEMCU_IP = "http://192.168.77.227:80"; // Change to your ESP32's IP



// MySQL database connection
const connection = mysql.createConnection({
    host: 'localhost',
    user: 'root',
    password: 'your_password',
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

 // JavaScript to handle modal functionality
 function openModal() {
    document.getElementById('pdfModal').style.display = 'block';
    document.getElementById('backdrop').style.display = 'block';
}

function closeModal() {
    document.getElementById('pdfModal').style.display = 'none';
    document.getElementById('backdrop').style.display = 'none';
}


app.post('/signup', async(req, res) => {
    try {
        const response = await axios.get(`${NODEMCU_IP}/get-time`);
        const currentTime = response.data.time || response.data; // depends on ESP JSON
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
                res.render('timer', { name, currentTime });
            });
    }

        //res.render('timer', { currentTime });
      } catch (error) {
        console.error("Error fetching time from NodeMCU:", error.message);
        res.render('timer', { name:"oye", currentTime: "Error fetching time" });
      }
      
    
});

// app.post('/signin', (req, res) => {
//     const { name, password } = req.body;
//     if (!name || !password) {
//         return res.status(400).send("Invalid Input");
//     }

//     connection.query('SELECT * FROM cuckoo WHERE User_Name = (?) AND Password = (?)', [name, password], (err, results) => {
//         if (err) throw err;

//         if (results.length > 0) {
//             const name = results[0].User_Name;
//             console.log(results);
//             res.render('timer', { name });
//         } else {
//             message = "Wrong credentials, try again";
//             res.render('index', { message });
//         }
//     });
//     //res.render('timer', {name:"Vansh"});
// });

app.post('/signin', async(req, res) => {
    try {
        const response = await axios.get(`${NODEMCU_IP}/get-time`);
        const currentTime = response.data.time || response.data; // depends on ESP JSON
        const { name, password } = req.body;
    if (!name || !password) {
        return res.status(400).send("Invalid Input");
    }

    if (connection.state === 'disconnected') {
        connection.connect(err => {
            if (err) {
                console.error('Error connecting to MySQL:', err);
                return res.status(500).send('Database connection error');
            }
        });
    }

    connection.query('SELECT * FROM cuckoo WHERE User_Name = (?) AND Password = (?)', [name, password], (err, results) => {
        if (err) {
            console.error(err);
            return res.status(500).send('Error executing query');
        }

        if (results.length > 0) {
            const userName = results[0].User_Name;
            console.log(results);
            res.render('timer', { name: userName, currentTime });
        } else {
            message = "Wrong credentials, try again";
            res.render('index', { message });
        }
    });

        //res.render('timer', { currentTime });
      } catch (error) {
        console.error("Error fetching time from NodeMCU:", error.message);
        res.render('timer', { name: "oye", currentTime: "Error fetching time" });
      }

});



  
  // API Route (optional: for AJAX fetch from frontend)
  app.get('/realtime', async (req, res) => {
    try {
      const response = await axios.get(`${NODEMCU_IP}/get-time`);
      res.json({ time: response.data.time || response.data });
    } catch (error) {
      console.error("Error fetching time from NodeMCU:", error.message);
      res.status(500).json({ error: "Unable to fetch time" });
    }
  });
  
  app.get('/time-match', async (req, res) => {
    console.log("⌛ Time matched! Received from client.");
  
    // Immediately send a response to client
    res.send("Server received time match!");
  
    // Now handle motor actions
    try {
        console.log("▶️ Moving motor forward for 5 seconds...");
        await fetch(`http://192.168.77.227:80/forward`, { method: 'GET' });
  
        // Wait for 5 seconds
        await new Promise(resolve => setTimeout(resolve, 5000));
  
        console.log("⏹️ Stopping motor for 50 seconds...");
        await fetch(`http://192.168.77.227:80/stop`, { method: 'GET' });
  
        // Wait for 50 seconds
        await new Promise(resolve => setTimeout(resolve, 50000));
  
        console.log("◀️ Moving motor backward for 5 seconds...");
        await fetch(`http://192.168.77.227:80/backward`, { method: 'GET' });
  
        // Wait for 5 seconds
        await new Promise(resolve => setTimeout(resolve, 5000));
  
        console.log("⏹️ Finally stopping motor after total 60 seconds...");
        await fetch(`http://192.168.77.227:80/stop`, { method: 'GET' });
  
    } catch (error) {
        console.error('Error during motor control sequence:', error);
    }
  });


  app.get('/weight', async(req, res)=>{
      try{
          const response2 = await axios.get(`${NODEMCU_IP}/weight`);
          const currentWeight = response2.data.weight || response2.data;
          console.log("Weight received-->");
          console.log(currentWeight);
          res.json({ weight: currentWeight});
  
      } catch (error) {
          console.error("Error fetching weight from NodeMCU:", error.message);
          res.status(500).json({ error: "Unable to fetch weight" });
        }
    })

    
  // Chatbot Route
 // Chatbot Route
app.post('/chatbot', (req, res) => {
    const userMessage = req.body.message.toLowerCase();

    let reply = "Sorry, I didn't understand that.";

    if (userMessage.includes('hello') || userMessage.includes('hi')) {
        reply = "Hello! How can I assist you today?";
    } else if (userMessage.includes('time')) {
        reply = "You can see the current time above! ⏰";
    } else if (userMessage.includes('medicine')) {
        reply = "Don't forget to set your medicine reminder times!";
    } else if (userMessage.includes('bye')) {
        reply = "Goodbye! Have a nice day!";
    }
    // Add more conditions if you want!

    res.json({ reply });
});






const PORT = process.env.PORT || 3003;
app.listen(PORT, () => console.log(`Server running on port ${PORT}`));