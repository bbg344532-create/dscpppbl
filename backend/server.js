const express = require('express');
const app = express();
const PORT = process.env.PORT || 5000;

app.use(express.json());

// TODO: mount route modules once implemented
// const emergencyRoutes = require('./routes/emergencyRoutes');
// app.use('/api/emergencies', emergencyRoutes);

app.get('/', (req, res) => {
    res.send('MedRoute backend is running.');
});

app.listen(PORT, () => {
    console.log(`MedRoute backend listening on port ${PORT}`);
});
