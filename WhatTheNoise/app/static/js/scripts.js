document.addEventListener('DOMContentLoaded', function() {
    // Get current year to display in footer
    var currentYearElement = document.getElementById('current-year');
    var currentYear = new Date().getFullYear();
    currentYearElement.textContent = currentYear;

    // Initlialize tooltips
    var tooltipTriggerList = [].slice.call(document.querySelectorAll('[data-bs-toggle="tooltip"]'))
    var tooltipList = tooltipTriggerList.map(function (tooltipTriggerEl) {
        return new bootstrap.Tooltip(tooltipTriggerEl)
    })
});

function toggleCheckbox(selectedId) {
    const checkboxes = ['portaudio', 'alsa'];
    checkboxes.forEach(id => {
        if (id !== selectedId) {
            document.getElementById(id).checked = false;
        }
    });
}

function validateForm() {
    const portaudioChecked = document.getElementById('portaudio').checked;
    const alsaChecked = document.getElementById('alsa').checked;
    const mic1 = document.getElementById('mic1').value;
    const mic2 = document.getElementById('mic2').value;

    if (!portaudioChecked && !alsaChecked) {
        alert('Debe seleccionar al menos una opción: Usar PortAudio o Usar ALSA.');
        return false;
    }

    if (mic1 == mic2) {
        alert('Debe seleccionar dos micrófonos diferentes.');
        return false
    }

    return true;
}