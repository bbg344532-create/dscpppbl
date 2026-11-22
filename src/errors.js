// An error that knows which HTTP status and error type to send back.
class ApiError extends Error {
  constructor(status, type, message) {
    super(message);
    this.status = status;
    this.type = type;
  }
}

// Turns an error reported by the C++ engine ({type, message}) into an ApiError.
function fromEngineError(err) {
  const status = {
    UnknownLocationError: 400,
    UnknownRoadError: 400,
    InvalidArgumentError: 400,
    UnknownIdError: 404,
    DuplicateIdError: 409,
  }[err.type] || 500;
  return new ApiError(status, err.type, err.message);
}

module.exports = { ApiError, fromEngineError };
