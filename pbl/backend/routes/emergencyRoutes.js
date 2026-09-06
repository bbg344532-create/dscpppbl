const express = require('express');
const router = express.Router();

// TODO: connect these to emergencyController once core engine is wired in

router.get('/', (req, res) => {
    res.send('List of emergencies - not yet implemented');
});

router.post('/', (req, res) => {
    res.send('Create new emergency - not yet implemented');
});

module.exports = router;
